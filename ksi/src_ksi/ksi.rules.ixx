module;

#include "../src/pre.h"

export module ksi.rules;

import <type_traits>;
import <cctype>;
import <cerrno>;
import <limits>;
export import ksi.tokens;

export namespace ksi {

	using namespace just::text_literals;
	using namespace std::literals::string_view_literals;

	namespace rules {


		struct state;

		using t_raw_const = t_text_value::const_pointer;
		using t_raw = t_text_value::pointer;

		using fn_parse = bool (*)(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data);

		struct line_info {
			// data
			t_raw_const		m_line_start;
			t_index			m_line = 1;
			t_index			m_tab_extra = 0;

			void next_line(t_raw_const p_line_start) {
				++m_line;
				m_line_start = p_line_start;
			}

			position pos(t_raw_const p_text) const { return {m_line, p_text - m_line_start + m_tab_extra +1}; }
		};

		struct state_base {
			// data
			line_info		m_line;
			t_int_ptr		m_tab_size = n_tab_size;

			void tab(t_raw_const p_text) {
				t_index v_extra = (p_text - m_line.m_line_start + m_line.m_tab_extra) % m_tab_size;
				m_line.m_tab_extra += m_tab_size -1 - v_extra;
			}
		};

		enum class nest {
			declarative,
			imperative
		};

		enum class kind {
			start,
			end,
			space,
			keep,
			special,
			n_literal,
			constant,
			variable,
			n_operator,
			category,
			type,
			function_name,
			separator
		};

		using flags_raw = just::t_uint_max;

		enum flags : flags_raw {
			flag_allow_plain	= 1 << 0,
			flag_was_refers		= 1 << 1,
			flag_was_extends	= 1 << 2,
			flag_was_colon		= 1 << 3,
		};

		//

		struct state :
			public state_base
		{
			// data
			fs::path		m_path;
			t_raw_const		m_text_pos;
			fn_parse		m_fn_parse;
			nest			m_nest;
			kind			m_kind = kind::start;
			flags_raw		m_flags = 0;
			t_int_ptr		m_loop_depth = 0;
			bool			m_was_space = false;
			bool			m_nice = false;
			bool			m_done = false;
			tokens::token_function_add::pointer		m_token_function_add = nullptr;

			state(const fs::path p_path, t_raw_const p_text_pos, fn_parse p_fn_parse, nest p_nest,
				t_int_ptr p_tab_size = n_tab_size
			) :
				state_base{{p_text_pos}, p_tab_size},
				m_path{p_path},
				m_text_pos{p_text_pos},
				m_fn_parse{p_fn_parse},
				m_nest{p_nest}
			{}

			position pos() const { return m_line.pos(m_text_pos); }

			bool flag_check(flags_raw p_flag) { return (m_flags & p_flag) == p_flag; }
			bool flag_check_any(flags_raw p_flag) { return m_flags & p_flag; }
			void flag_set(flags_raw p_flag) { m_flags |= p_flag; }
			void flag_unset(flags_raw p_flag) { m_flags &= ~p_flag; }

			void done_nice() {
				m_nice = true;
				m_done = true;
			}

			void done() {
				m_done = true;
			}
		};

		//

		template <typename, typename = void>
		struct is_rule_inner : public std::false_type {};

		template <typename T>
		struct is_rule_inner<T, std::void_t<typename T::t_data> > : public std::true_type {};

		template <typename T>
		constexpr bool is_rule_inner_v = is_rule_inner<T>::value;

		//

		struct t_eof {
			static constexpr kind s_kind{ kind::end };
			static t_text_value name() { return "t_eof"_jt; }
			static bool check(state & p_state) { return true; }

			struct t_data {
				bool parse(state & p_state, tokens::nest_tokens & p_tokens, log_pointer p_log) {
					return *p_state.m_text_pos == '\0';
				}

				void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {
					p_state.done_nice();
				}
			};
		};

		struct t_space {
			static constexpr kind s_kind{ kind::space };
			static t_text_value name() { return "t_space"_jt; }
			static bool check(state & p_state) { return !p_state.m_was_space; }

			enum class nest_comments { none, line, multiline };

			struct t_data {
				static bool take_new_line(state & p_state, t_raw_const & p_text_pos, bool & p_continue) {
					switch( *p_text_pos ) {
					case '\n':
						p_continue = true;
						++p_text_pos;
						p_state.m_line.next_line(p_text_pos);
						return true;
					case '\r':
						p_continue = true;
						if( p_text_pos[1] == '\n' ) { p_text_pos += 2; }
						else { ++p_text_pos; }
						p_state.m_line.next_line(p_text_pos);
						return true;
					}
					return false;
				}

				static bool take_multiline_comment_open(t_raw_const & p_text_pos, t_index & p_depth, bool & p_continue) {
					if( *p_text_pos == '/' && p_text_pos[1] == '*' ) {
						p_continue = true;
						++p_depth;
						p_text_pos += 2;
						return true;
					}
					return false;
				}

				bool parse(state & p_state, tokens::nest_tokens & p_tokens, log_pointer p_log) {
					t_raw_const v_text_pos = p_state.m_text_pos;
					//
					{
						bool v_continue;
						nest_comments v_nest = nest_comments::none;
						t_index v_depth = 0;
						do {
							v_continue = false;
							if( *v_text_pos == '\0' ) break;
							bool v_new_line = take_new_line(p_state, v_text_pos, v_continue);
							switch( v_nest ) {
							case nest_comments::none :
								if( take_multiline_comment_open(v_text_pos, v_depth, v_continue) ) {
									v_nest = nest_comments::multiline;
								} else {
									switch( *v_text_pos ) {
									case '\t':
										p_state.tab(v_text_pos);
									case ' ':
										v_continue = true;
										++v_text_pos;
										break;
									case '-':
										if( v_text_pos[1] == '-' ) {
											v_continue = true;
											v_text_pos += 2;
											v_nest = nest_comments::line;
										}
										break;
									}
								}
								break;
							case nest_comments::line :
								if( v_new_line ) { v_nest = nest_comments::none; break; }
								v_continue = true;
								++v_text_pos;
								break;
							case nest_comments::multiline :
								take_multiline_comment_open(v_text_pos, v_depth, v_continue);
								if( *v_text_pos == '*' && v_text_pos[1] == '/' ) {
									v_continue = true;
									v_text_pos += 2;
									--v_depth;
									if( v_depth == 0 ) { v_nest = nest_comments::none; }
								} else {
									v_continue = true;
									++v_text_pos;
								}
								break;
							}
						} while( v_continue );
					}
					if( p_state.m_text_pos != v_text_pos ) {
						p_state.m_text_pos = v_text_pos;
						return true;
					}
					return false;
				}

				void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {}
			};
		};

		//

		template <typename ... T_items>
		struct first_type;

		template <typename T_first, typename ... T_rest>
		struct first_type<T_first, T_rest ...> {
			using type = T_first;

			template <bool C_flag, template <bool, typename ...> typename T_template>
			using type_rest = T_template<C_flag, T_rest ...>;
		};

		template <typename ... T_items>
		using first_type_t = first_type<T_items ...>::type;

		template <bool C_none_match_done, typename ... T_rules>
		struct rule_alt {
			using t_first = first_type_t<T_rules ...>;
			using t_items = std::vector<t_text_value>;

			static constexpr bool s_take_rest = sizeof...(T_rules) > 1;
			
			using t_rest = std::conditional_t<s_take_rest,
				typename first_type<T_rules ...>::template type_rest<C_none_match_done, rule_alt>,
				void
			>;

			static t_text_value message(state & p_state) {
				t_items v_items;
				add_items(p_state, v_items);
				t_text_value v_text = just::implode_items<t_char, t_text_value>(v_items, ", "sv);
				std::string_view v_symbol = "symbol";
				if( *p_state.m_text_pos == '\0' ) { v_symbol = "EOF"; }
				return just::implode<t_char>({"parse error: Unexpected "sv, v_symbol, ". Expected ("sv, v_text, ")"sv});
			}

			using fn_message = decltype(&message);

			static void add_items(state & p_state, t_items & p_items) {
				if( t_first::check(p_state) ) {
					p_items.push_back(t_first::name() );
				}
				if constexpr( s_take_rest ) {
					t_rest::add_items(p_state, p_items);
				}
			}

			static bool parse_inner(state & p_state, tokens::nest_tokens & p_tokens,
				prepare_data::pointer p_data, fn_message p_fn
			) {
				if( t_first::check(p_state) ) {
					bool v_nice = false;
					if constexpr( is_rule_inner_v<t_first> ) {
						typename t_first::t_data v_data;
						if( v_data.parse(p_state, p_tokens, p_data->m_log) ) {
							v_data.action(p_state, p_tokens, p_data);
							v_nice = true;
						}
					} else if( t_first::parse(p_state, p_tokens, p_data) ) {
						v_nice = true;
					}
					if( v_nice ) {
						p_state.m_was_space = std::is_same_v<t_first, t_space>;
						if constexpr( ! just::is_one_of(t_first::s_kind, kind::space, kind::keep) ) {
							p_state.m_kind = t_first::s_kind;
						}
						return true;
					}
				}
				if constexpr( s_take_rest ) {
					if( !p_state.m_done ) {
						return t_rest::parse_inner(p_state, p_tokens, p_data, p_fn);
					}
				} else if constexpr( C_none_match_done ) {
					p_data->m_log->add({p_state.m_path, p_fn(p_state), p_state.pos()});
					p_state.done();
				}
				return false;
			}

			static bool parse(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {
				return parse_inner(p_state, p_tokens, p_data, &message);
			}
		};

		//

		template <t_char ... C_char>
		struct is_char {
			bool parse(state & p_state, tokens::nest_tokens & p_tokens, log_pointer p_log) {
				if( just::is_one_of(*p_state.m_text_pos, C_char ...) ) {
					++p_state.m_text_pos;
					return true;
				}
				return false;
			}
		};

		template <just::fixed_string C_text>
		struct is_keyword {
			bool parse(state & p_state, tokens::nest_tokens & p_tokens, log_pointer p_log) {
				if( just::text_traits::cmp_n(p_state.m_text_pos, C_text.m_text, C_text.s_length) == 0 ) {
					p_state.m_text_pos += C_text.s_length;
					return true;
				}
				return false;
			}
		};

		template <bool C_allow_std = false>
		struct traits {
			static bool impl_take_name(t_raw_const & p_text_pos) {
				if( std::isalpha(*p_text_pos) ) {
					++p_text_pos;
					while( *p_text_pos == '_' || std::isalnum(*p_text_pos) ) { ++p_text_pos; }
					if constexpr( C_allow_std ) {
						if( *p_text_pos == '#' ) { ++p_text_pos; }
					}
					return true;
				}
				return false;
			}

			static bool take_name(state & p_state, t_text_value & p_name, position & p_pos) {
				t_raw_const v_name_end = p_state.m_text_pos;
				if( impl_take_name(v_name_end) ) {
					p_pos = p_state.pos();
					p_name = just::text_traits::from_range(p_state.m_text_pos, v_name_end);
					p_state.m_text_pos = v_name_end;
					return true;
				}
				return false;
			}

			static bool take_name_with_prefix(state & p_state, t_char p_prefix, t_text_value & p_name, position & p_pos) {
				if( *p_state.m_text_pos != p_prefix ) return false;
				t_raw_const v_name_end = p_state.m_text_pos +1;
				if( impl_take_name(v_name_end) ) {
					p_pos = p_state.pos();
					p_name = just::text_traits::from_range(p_state.m_text_pos, v_name_end);
					p_state.m_text_pos = v_name_end;
					return true;
				}
				return false;
			}
		};

		template <bool C_allow_std, t_char T_prefix>
		struct is_entity
		{
			// data
			entity_info		m_entity;

			bool parse(state & p_state, tokens::nest_tokens & p_tokens, log_pointer p_log) {
				bool ret = traits<C_allow_std>::take_name_with_prefix(p_state, T_prefix, m_entity.m_name, m_entity.m_pos);
				if( ret ) {
					position v_pos;
					if( traits<C_allow_std>::take_name_with_prefix(p_state, '@', m_entity.m_module_name, v_pos) ) {}
					else if( *p_state.m_text_pos == '@' ) {
						++p_state.m_text_pos;
						m_entity.m_module_name = "@"_jt;
					}
				}
				return ret;
			}
		};

		template <bool C_allow_std>
		struct is_category :
			public is_entity<C_allow_std, '_'>
		{};

		template <bool C_allow_std>
		struct is_type :
			public is_entity<C_allow_std, '$'>
		{};

		//

		struct all {

			struct t_module_name {
				static constexpr kind s_kind{ kind::special };
				static t_text_value name() { return "t_module_name"_jt; }
				static bool check(state & p_state) { return true; }

				struct t_data {
					// data
					t_text_value	m_name;

					bool parse(state & p_state, tokens::nest_tokens & p_tokens, log_pointer p_log) {
						position v_pos;
						return traits<>::take_name_with_prefix(p_state, '@', m_name, v_pos);
					}

					void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {
						p_tokens.m_cats.append( new tokens::token_module_name(m_name) );
						p_state.m_fn_parse = &rule_decl::parse;
					}
				};
			};

			// category

			struct t_category_def_name {
				static constexpr kind s_kind{ kind::special };
				static t_text_value name() { return "t_category_def_name"_jt; }
				static bool check(state & p_state) { return true; }

				struct t_data {
					// data
					t_text_value	m_name;
					position		m_pos;
					bool			m_is_local = false;

					bool parse(state & p_state, tokens::nest_tokens & p_tokens, log_pointer p_log) {
						bool ret = traits<>::take_name_with_prefix(p_state, '_', m_name, m_pos);
						if( ret && *p_state.m_text_pos == '@' ) {
							++p_state.m_text_pos;
							m_is_local = true;
						}
						return ret;
					}

					void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {
						p_tokens.m_cats.append(
							new tokens::token_category_add({m_name, m_is_local, {p_state.m_path, m_pos}})
						);
						p_state.m_fn_parse = &rule_category_open::parse;
					}
				};
			};

			struct t_category_open {
				static constexpr kind s_kind{ kind::start };
				static t_text_value name() { return "t_category_open"_jt; }
				static bool check(state & p_state) { return true; }

				struct t_data :
					public is_char<'('>
				{
					void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {
						p_state.m_fn_parse = &rule_category_inside::parse;
					}
				};
			};

			struct t_category_close {
				static constexpr kind s_kind{ kind::special };
				static t_text_value name() { return "t_category_close"_jt; }
				static bool check(state & p_state) { return true; }

				struct t_data :
					public is_char<')'>
				{
					void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {
						p_state.m_fn_parse = &rule_decl::parse;
					}
				};
			};

			struct t_category_includes_name {
				static constexpr kind s_kind{ kind::special };
				static t_text_value name() { return "t_category_includes_name"_jt; }
				static bool check(state & p_state) { return true; }

				struct t_data :
					is_category<false>
				{
					void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {
						p_tokens.m_cats.append( new tokens::token_category_add_base(p_state.m_path, m_entity) );
					}
				};
			};

			// type

			struct t_type_def_name {
				static constexpr kind s_kind{ kind::special };
				static t_text_value name() { return "t_type_def_name"_jt; }
				static bool check(state & p_state) { return true; }

				struct t_data {
					// data
					t_text_value	m_name;
					position		m_pos;
					bool			m_is_local = false;

					bool parse(state & p_state, tokens::nest_tokens & p_tokens, log_pointer p_log) {
						bool ret = traits<>::take_name_with_prefix(p_state, '$', m_name, m_pos);
						if( ret && *p_state.m_text_pos == '@' ) {
							++p_state.m_text_pos;
							m_is_local = true;
						}
						return ret;
					}

					void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {
						p_tokens.m_types.append(
							new tokens::token_type_add({m_name, m_is_local, {p_state.m_path, m_pos}})
						);
						p_state.m_fn_parse = &rule_type_kind::parse;
					}
				};
			};

			// refers

			struct t_kw_refers {
				static constexpr kind s_kind{ kind::special };
				static t_text_value name() { return "t_kw_refers"_jt; }
				static bool check(state & p_state) { return ! p_state.flag_check_any(flag_was_refers | flag_was_extends); }

				struct t_data :
					public is_keyword<"refers">
				{
					void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {
						p_state.flag_set(flag_was_refers);
						p_state.m_fn_parse = &rule_refers_open::parse;
					}
				};
			};

			struct t_refers_open {
				static constexpr kind s_kind{ kind::start };
				static t_text_value name() { return "t_refers_open"_jt; }
				static bool check(state & p_state) { return true; }

				struct t_data :
					public is_char<'('>
				{
					void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {
						p_state.m_fn_parse = &rule_refers_inside::parse;
					}
				};
			};

			struct t_refers_close {
				static constexpr kind s_kind{ kind::special };
				static t_text_value name() { return "t_refers_close"_jt; }
				static bool check(state & p_state) { return p_state.m_kind != kind::start; }

				struct t_data :
					public is_char<')'>
				{
					void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {
						p_state.m_fn_parse = &rule_type_kind::parse;
					}
				};
			};

			struct t_refers_category_name {
				static constexpr kind s_kind{ kind::special };
				static t_text_value name() { return "t_refers_category_name"_jt; }
				static bool check(state & p_state) { return true; }

				struct t_data :
					public is_category<false>
				{
					void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {
						p_tokens.m_types.append( new tokens::token_type_add_category(m_entity) );
					}
				};
			};

			// extends

			struct t_kw_extends {
				static constexpr kind s_kind{ kind::special };
				static t_text_value name() { return "t_kw_extends"_jt; }
				static bool check(state & p_state) { return ! p_state.flag_check(flag_was_extends); }

				struct t_data :
					public is_keyword<"extends">
				{
					void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {
						p_state.flag_set(flag_was_extends);
						p_state.m_fn_parse = &rule_extends_open::parse;
					}
				};
			};

			struct t_extends_open {
				static constexpr kind s_kind{ kind::start };
				static t_text_value name() { return "t_extends_open"_jt; }
				static bool check(state & p_state) { return true; }

				struct t_data :
					public is_char<'('>
				{
					void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {
						p_state.m_fn_parse = &rule_extends_inside::parse;
					}
				};
			};

			struct t_extends_close {
				static constexpr kind s_kind{ kind::special };
				static t_text_value name() { return "t_extends_close"_jt; }
				static bool check(state & p_state) { return p_state.m_kind != kind::start; }

				struct t_data :
					public is_char<')'>
				{
					void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {
						p_state.m_fn_parse = &rule_type_kind::parse;
					}
				};
			};

			struct t_extends_type_name {
				static constexpr kind s_kind{ kind::special };
				static t_text_value name() { return "t_extends_type_name"_jt; }
				static bool check(state & p_state) { return true; }

				struct t_data :
					public is_type<false>
				{
					void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {
						p_tokens.m_types.append( new tokens::token_type_add_base(m_entity) );
					}
				};
			};

			// struct

			struct t_kw_struct {
				static constexpr kind s_kind{ kind::special };
				static t_text_value name() { return "t_kw_struct"_jt; }
				static bool check(state & p_state) { return true; }

				struct t_data :
					public is_keyword<"struct">
				{
					void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {
						p_tokens.m_types.append(new tokens::token_struct_add{p_state.m_path});
						p_state.m_fn_parse = &rule_struct_open::parse;
					}
				};
			};

			struct t_struct_open {
				static constexpr kind s_kind{ kind::start };
				static t_text_value name() { return "t_struct_open"_jt; }
				static bool check(state & p_state) { return true; }

				struct t_data :
					public is_char<'('>
				{
					void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {
						p_state.m_fn_parse = &rule_struct_inside::parse;
					}
				};
			};

			struct t_struct_close {
				static constexpr kind s_kind{ kind::special };
				static t_text_value name() { return "t_struct_close"_jt; }
				static bool check(state & p_state) { return p_state.m_kind != kind::n_operator; }

				struct t_data :
					public is_char<')'>
				{
					void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {
						p_tokens.m_types.append( new tokens::token_struct_end() );
						p_state.m_fn_parse = &rule_decl::parse;
						p_state.flag_unset(flag_was_refers | flag_was_extends);
					}
				};
			};

			struct t_struct_prop_name {
				static constexpr kind s_kind{ kind::variable };
				static t_text_value name() { return "t_struct_prop_name"_jt; }
				static bool check(state & p_state) {
					if( p_state.m_kind == kind::n_literal && (p_state.m_was_space == false) ) { return false; }
					return p_state.m_kind != kind::n_operator;
				}

				struct t_data
				{
					// data
					position		m_pos;
					t_text_value	m_name;

					bool parse(state & p_state, tokens::nest_tokens & p_tokens, log_pointer p_log) {
						return traits<>::take_name(p_state, m_name, m_pos);
					}

					void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {
						p_tokens.m_types.append( new tokens::token_struct_prop_name({p_state.m_path, m_pos}, m_name) );
					}
				};
			};

			struct t_struct_prop_separator {
				static constexpr kind s_kind{ kind::separator };
				static t_text_value name() { return "t_struct_prop_separator"_jt; }
				static bool check(state & p_state) {
					return ! just::is_one_of(p_state.m_kind, kind::start, kind::n_operator, kind::separator);
				}

				struct t_data :
					public is_char<',', ';'>
				{
					void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {}
				};
			};

			struct t_struct_prop_assign {
				static constexpr kind s_kind{ kind::n_operator };
				static t_text_value name() { return "t_struct_prop_assign"_jt; }
				static bool check(state & p_state) {
					return p_state.m_kind == kind::variable;
				}

				struct t_data :
					public is_char<'='>
				{
					void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {}
				};
			};

			// function

			struct t_function_def_name {
				static constexpr kind s_kind{ kind::start };
				static t_text_value name() { return "t_function_def_name"_jt; }
				static bool check(state & p_state) { return true; }

				struct t_data {
					// data
					t_text_value	m_name;
					position		m_pos;
					bool			m_is_local = false;

					bool parse(state & p_state, tokens::nest_tokens & p_tokens, log_pointer p_log) {
						bool ret = traits<>::take_name_with_prefix(p_state, '&', m_name, m_pos);
						if( ret && *p_state.m_text_pos == '@' ) {
							++p_state.m_text_pos;
							m_is_local = true;
						}
						return ret;
					}

					void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {
						p_tokens.m_functions.append(
							p_state.m_token_function_add = new tokens::token_function_add({
								m_name, m_is_local, {p_state.m_path, m_pos}
							})
						);
						p_state.m_fn_parse = &rule_function_arg::parse;
					}
				};
			};

			struct t_function_overload_by_category {
				static constexpr kind s_kind{ kind::category };
				static t_text_value name() { return "t_function_overload_by_category"_jt; }
				static bool check(state & p_state) { return p_state.m_kind == kind::start; }

				struct t_data :
					is_category<true>
				{
					void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {
						p_data->m_late.m_functions.append(
							p_state.m_token_function_add->m_late_token =
							new tokens::late_token_function_body_add_over_category(p_state.m_path, m_entity)
						);
					}
				};
			};

			struct t_function_overload_by_type {
				static constexpr kind s_kind{ kind::type };
				static t_text_value name() { return "t_function_overload_by_type"_jt; }
				static bool check(state & p_state) { return p_state.m_kind == kind::start; }

				struct t_data :
					is_type<true>
				{
					void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {
						p_data->m_late.m_functions.append(
							p_state.m_token_function_add->m_late_token =
							new tokens::late_token_function_body_add_over_type(p_state.m_path, m_entity)
						);
					}
				};
			};

			struct t_function_arg {
				static constexpr kind s_kind{ kind::variable };
				static t_text_value name() { return "t_function_arg"_jt; }
				static bool check(state & p_state) { return true; }

				struct t_data {
					// data
					t_text_value	m_name;

					bool parse(state & p_state, tokens::nest_tokens & p_tokens, log_pointer p_log) {
						position v_pos;
						return traits<>::take_name(p_state, m_name, v_pos);
					}

					void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {
						if( p_state.m_kind == kind::start ) {
							p_data->m_late.m_functions.append(
								p_state.m_token_function_add->m_late_token =
								new tokens::late_token_function_body_add_over_common()
							);
						}
						p_data->m_late.m_functions.append(
							new tokens::late_token_function_add_arg(m_name)
						);
						p_state.m_fn_parse = &rule_function_open::parse;
					}
				};
			};

			struct t_function_open {
				static constexpr kind s_kind{ kind::start };
				static t_text_value name() { return "t_function_open"_jt; }
				static bool check(state & p_state) { return true; }

				struct t_data :
					public is_char<'('>
				{
					void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {
						p_state.m_fn_parse = &rule_function_inside::parse;
					}
				};
			};

			struct t_function_close {
				static constexpr kind s_kind{ kind::special };
				static t_text_value name() { return "t_function_close"_jt; }
				static bool check(state & p_state) { return true; }

				struct t_data :
					public is_char<')'>
				{
					void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {
						p_state.m_fn_parse = &rule_decl::parse;
					}
				};
			};

			// literal

			struct t_literal_null {
				static constexpr kind s_kind{ kind::n_literal };
				static t_text_value name() { return "t_literal_null"_jt; }
				static bool check(state & p_state) { return true; }

				struct t_data :
					public is_keyword<"null">
				{
					void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {
						if( p_state.m_nest != nest::declarative ) { p_tokens.put_literal(var::any_var{}); }
					}
				};
			};

			struct t_literal_all {
				static constexpr kind s_kind{ kind::n_literal };
				static t_text_value name() { return "t_literal_all"_jt; }
				static bool check(state & p_state) { return true; }

				struct t_data :
					public is_keyword<"all">
				{
					void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {
						if( p_state.m_nest != nest::declarative ) { p_tokens.put_literal(var::variant_all{}); }
					}
				};
			};

			struct t_literal_false {
				static constexpr kind s_kind{ kind::n_literal };
				static t_text_value name() { return "t_literal_false"_jt; }
				static bool check(state & p_state) { return true; }

				struct t_data :
					public is_keyword<"false">
				{
					void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {
						p_tokens.put_literal(false);
					}
				};
			};

			struct t_literal_true {
				static constexpr kind s_kind{ kind::n_literal };
				static t_text_value name() { return "t_literal_true"_jt; }
				static bool check(state & p_state) { return true; }

				struct t_data :
					public is_keyword<"true">
				{
					void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {
						p_tokens.put_literal(true);
					}
				};
			};

			struct t_literal_int {
				static constexpr kind s_kind{ kind::n_literal };
				static t_text_value name() { return "t_literal_int"_jt; }
				static bool check(state & p_state) { return true; }

				struct t_data {
					// data
					t_integer	m_value;

					static bool is_separator(t_char p_char) { return just::is_one_of(p_char, '_', '\''); }

					static bool is_digit_binary(t_char p_char) { return just::is_one_of(p_char, '0', '1'); }
					static bool is_digit_octal(t_char p_char) { return p_char >= '0' && p_char < '8'; }

					static bool is_digit(t_char p_char, int p_radix) {
						switch( p_radix ) {
						case 2: return is_digit_binary(p_char);
						case 8: return is_digit_octal(p_char);
						case 16: return std::isxdigit(p_char);
						}
						return std::isdigit(p_char);
					}

					bool parse(state & p_state, tokens::nest_tokens & p_tokens, log_pointer p_log) {
						t_raw_const v_pos_start = p_state.m_text_pos, v_pos_end;
						std::string_view v_prefix = "0"sv;
						bool v_is_negative = false;
						switch( *v_pos_start ) {
						case '-':
							++v_pos_start;
							v_prefix = "-0"sv;
							v_is_negative = true;
							break;
						case '+':
							++v_pos_start;
							break;
						}
						int v_radix = 10;
						v_pos_end = v_pos_start;
						if( *v_pos_start == '0' ) {
							switch( v_pos_start[1] ) {
							case 'b':
								v_radix = 2;
								v_pos_start += 2;
								if( *v_pos_start == '_' ) { ++v_pos_start; }
								v_pos_end = v_pos_start;
								while( is_digit_binary(*v_pos_end) ) { ++v_pos_end; }
								break;
							case 'o':
								v_radix = 8;
								v_pos_start += 2;
								if( *v_pos_start == '_' ) { ++v_pos_start; }
								v_pos_end = v_pos_start;
								while( is_digit_octal(*v_pos_end) ) { ++v_pos_end; }
								break;
							case 'h':
								v_radix = 16;
								v_pos_start += 2;
								if( *v_pos_start == '_' ) { ++v_pos_start; }
								v_pos_end = v_pos_start;
								while( std::isxdigit(*v_pos_end) ) { ++v_pos_end; }
								break;
							}
						}
						if( v_radix == 10 ) {
							while( std::isdigit(*v_pos_end) ) { ++v_pos_end; }
						}
						if( v_pos_start == v_pos_end && v_radix == 10 ) { return false; }
						t_text_value v_text = just::text_traits::from_range(v_pos_start, v_pos_end);
						v_text = just::implode<t_char>({v_prefix, v_text});
						//
						v_pos_start = v_pos_end;
						while( is_separator(*v_pos_start) && is_digit(v_pos_start[1], v_radix) ) {
							++v_pos_start;
							v_pos_end = v_pos_start +1;
							while( is_digit(*v_pos_end, v_radix) ) { ++v_pos_end; }
							//t_text_value v_text_ending = just::text_traits::from_range(v_pos_start, v_pos_end);
							std::string_view v_text_ending{v_pos_start, static_cast<t_size>(v_pos_end - v_pos_start)};
							v_text = just::implode<t_char>({v_text, v_text_ending});
						}
						errno = 0;
						m_value = std::strtoll(v_text.data(), nullptr, v_radix);
						if( errno == ERANGE ) {
							t_text_value v_message = (v_is_negative ?
								"warning: Integer literal is out of bounds so $int#.max# will be used instead."_jt :
								"warning: Integer literal is out of bounds so $int#.min# will be used instead."_jt
							);
							p_log->add({p_state.m_path, v_message, p_state.pos()});
						}
						p_state.m_text_pos = v_pos_end;
						return true;
					} // fn

					void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {
						//just::g_console << m_value << just::g_new_line;
						p_tokens.put_literal(m_value);
					}
				};
			};

			struct t_literal_float {
				static constexpr kind s_kind{ kind::n_literal };
				static t_text_value name() { return "t_literal_float"_jt; }
				static bool check(state & p_state) { return true; }

				struct t_data {
					// data
					t_floating	m_value;

					void maybe_separator(t_text_value & p_text, t_raw_const & p_pos_start, t_raw_const & p_pos_end) {
						while( t_literal_int::t_data::is_separator(*p_pos_start) && std::isdigit(p_pos_start[1]) ) {
							++p_pos_start;
							p_pos_end = p_pos_start +1;
							while( std::isdigit(*p_pos_end) ) { ++p_pos_end; }
							//t_text_value v_text_ending = just::text_traits::from_range(p_pos_start, p_pos_end);
							std::string_view v_text_ending{p_pos_start, static_cast<t_size>(p_pos_end - p_pos_start)};
							p_text = just::implode<t_char>({p_text, v_text_ending});
						}
					}

					bool parse(state & p_state, tokens::nest_tokens & p_tokens, log_pointer p_log) {
						t_raw_const v_pos_start = p_state.m_text_pos, v_pos_end;
						//std::string_view v_prefix = ""sv;
						bool v_is_negative = false;
						switch( *v_pos_start ) {
						case '-':
							++v_pos_start;
							//v_prefix = "-"sv;
							v_is_negative = true;
							break;
						case '+':
							++v_pos_start;
							break;
						}
						v_pos_end = v_pos_start;
						while( std::isdigit(*v_pos_end) ) { ++v_pos_end; }
						if( v_pos_end == v_pos_start ) { return false; }
						t_text_value v_text = just::text_traits::from_range(p_state.m_text_pos, v_pos_end);
						bool v_was_dot = false, v_was_e = false;
						v_pos_start = v_pos_end;
						maybe_separator(v_text, v_pos_start, v_pos_end);
						if( *v_pos_end == '.' && std::isdigit(v_pos_end[1]) ) {
							v_was_dot = true;
							v_pos_start = v_pos_end;
							v_pos_end += 2;
							while( std::isdigit(*v_pos_end) ) { ++v_pos_end; }
							std::string_view v_text_ending{v_pos_start, static_cast<t_size>(v_pos_end - v_pos_start)};
							v_text = just::implode<t_char>({v_text, v_text_ending});
						}
						v_pos_start = v_pos_end;
						maybe_separator(v_text, v_pos_start, v_pos_end);
						if( *v_pos_end == 'e' && just::is_one_of(v_pos_end[1], '-', '+') && std::isdigit(v_pos_end[2]) ) {
							v_was_e = true;
							v_pos_start = v_pos_end;
							v_pos_end += 3;
							while( std::isdigit(*v_pos_end) ) { ++v_pos_end; }
							std::string_view v_text_ending{v_pos_start, static_cast<t_size>(v_pos_end - v_pos_start)};
							v_text = just::implode<t_char>({v_text, v_text_ending});
						}
						v_pos_start = v_pos_end;
						maybe_separator(v_text, v_pos_start, v_pos_end);
						if( v_was_e == false && v_was_dot == false ) { return false; }
						errno = 0;
						m_value = std::strtod(v_text.data(), nullptr);
						if( errno == ERANGE ) {
							m_value = std::numeric_limits<t_floating>::infinity();
							t_text_value v_message = "warning: Floating point literal is out of bounds"
							" so $float#.infinity# will be used instead."_jt;
							if( v_is_negative ) {
								m_value = -m_value;
								v_message = "warning: Floating point literal is out of bounds"
								" so $float#.infinity_negative# will be used instead."_jt;
							}
							p_log->add({p_state.m_path, v_message, p_state.pos()});
						}
						p_state.m_text_pos = v_pos_end;
						return true;
					} // fn

					void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {
						//just::g_console << m_value << just::g_new_line;
						p_tokens.put_literal(m_value);
					}
				};
			};

			struct rule_literal :
				public rule_alt<false,
					t_literal_null,
					t_literal_all,
					t_literal_false,
					t_literal_true,
					t_literal_float,
					t_literal_int
				>
			{
				static constexpr kind s_kind{ kind::keep };
				static t_text_value name() { return "rule_literal"_jt; }

				static bool check(state & p_state) {
					return (p_state.m_nest == nest::declarative) ? (p_state.m_kind == kind::n_operator) : true;
				}
			};

			//

			struct rule_module_name :
				public rule_alt<true, t_space, t_module_name> {};

			struct rule_decl :
				public rule_alt<true, t_space, t_eof,
					t_category_def_name,
					t_type_def_name,
					t_function_def_name
				>
			{};

			struct rule_category_open :
				public rule_alt<true, t_space, t_category_open> {};

			struct rule_category_inside :
				public rule_alt<true, t_space, t_category_close, t_category_includes_name> {};

			struct rule_type_kind :
				public rule_alt<true, t_space, t_kw_refers, t_kw_extends, t_kw_struct> {};

			struct rule_refers_open :
				public rule_alt<true, t_space, t_refers_open> {};

			struct rule_refers_inside :
				public rule_alt<true, t_space, t_refers_close, t_refers_category_name> {};

			struct rule_extends_open :
				public rule_alt<true, t_space, t_extends_open> {};

			struct rule_extends_inside :
				public rule_alt<true, t_space, t_extends_close, t_extends_type_name> {};

			struct rule_struct_open :
				public rule_alt<true, t_space, t_struct_open> {};

			struct rule_struct_inside :
				public rule_alt<true, t_space,
					t_struct_close,
					t_struct_prop_name,
					t_struct_prop_assign,
					rule_literal,
					t_struct_prop_separator
				>
			{};

			struct rule_function_arg :
				public rule_alt<true, t_space,
					t_function_overload_by_category,
					t_function_overload_by_type,
					t_function_arg
				>
			{};

			struct rule_function_open :
				public rule_alt<true, t_space, t_function_open> {};

			struct rule_function_inside :
				public rule_alt<true, t_space, t_function_close> {};

		};

		bool parse_declarative(
			prepare_data::pointer p_data,
			fs::path p_path,
			const t_text_value & p_contents,
			tokens::nest_tokens & p_tokens,
			log_pointer p_log,
			t_int_ptr p_tab_size = n_tab_size
		) {
			state v_state{p_path, p_contents.data(), &all::rule_module_name::parse, nest::declarative, p_tab_size};
			do {
				v_state.m_fn_parse(v_state, p_tokens, p_data);
			} while( ! v_state.m_done );
			return v_state.m_nice;
		}

	} // ns

} // ns