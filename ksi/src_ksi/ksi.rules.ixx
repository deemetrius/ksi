module;

#include "../src/pre.h"

export module ksi.rules;

import <type_traits>;
import <cctype>;
export import ksi.tokens;

export namespace ksi {

	using namespace just::text_literals;

	namespace rules {

		struct state;

		using t_position = just::t_int_max;
		using t_char = t_text_value::type;
		using t_raw_const = t_text_value::const_pointer;
		using t_raw = t_text_value::pointer;

		using fn_parse = bool (*)(state & p_state, tokens::nest_tokens & p_tokens, log_pointer p_log);

		struct line_info {
			// data
			t_raw_const		m_line_start;
			t_position		m_line = 1;

			void next_line(t_raw_const p_line_start) {
				++m_line;
				m_line_start = p_line_start;
			}

			position pos(t_raw_const p_text) const { return {m_line, p_text - m_line_start +1}; }
		};

		enum class nest {
			declarative,
			imperative
		};

		enum class kind {
			start,
			n_literal,
			constant,
			variable,
			n_operator,
			function_name,
			separator
		};

		using flags_raw = just::t_uint_max;

		enum flags : flags_raw {
			flag_allow_plain	= 1 << 0,
			flag_was_colon		= 1 << 1
		};

		//

		struct state {
			// data
			fs::path		m_path;
			t_raw_const		m_text_pos;
			line_info		m_line;
			fn_parse		m_fn_parse;
			nest			m_nest;
			kind			m_kind = kind::start;
			flags_raw		m_flags = 0;
			t_position		m_loop_depth = 0;
			bool			m_was_space = false;
			bool			m_nice = false;
			bool			m_done = false;

			state(const fs::path p_path, t_raw_const p_text_pos, fn_parse p_fn_parse, nest p_nest) :
				m_path{p_path},
				m_text_pos{p_text_pos},
				m_line{p_text_pos},
				m_fn_parse{p_fn_parse},
				m_nest{p_nest}
			{}

			position pos() const { return m_line.pos(m_text_pos); }

			void done_nice() {
				m_nice = true;
				m_done = true;
			}

			void done() {
				m_done = true;
			}
		};

		//

		struct t_eof {
			static t_text_value name() { return "t_eof"_jt; }

			static bool check(state & p_state) { return true; }

			struct t_data {
				bool parse(state & p_state, tokens::nest_tokens & p_tokens, log_pointer p_log) {
					return *p_state.m_text_pos == '\0';
				}

				void action(state & p_state, tokens::nest_tokens & p_tokens, log_pointer p_log) {
					p_state.done_nice();
				}
			};
		};

		struct t_space {
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

				static bool take_multiline_comment_open(t_raw_const & p_text_pos, t_position & p_depth, bool & p_continue) {
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
						t_position v_depth = 0;
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
									case ' ':
									case '\t':
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

				void action(state & p_state, tokens::nest_tokens & p_tokens, log_pointer p_log) {}
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

		/*template <typename T_last>
		struct first_type<T_last> {
			using type = T_last;

			template <bool C_flag, template <bool, typename ...> typename T_template>
			using type_rest = T_template<C_flag>;
		};*/

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
				t_text_value v_text = just::implode_items<t_char, t_text_value>(v_items, ", ");
				std::string_view v_symbol = "symbol";
				if( *p_state.m_text_pos == '\0' ) { v_symbol = "EOF"; }
				return just::implode<t_char>({"parse error: Unexpected ", v_symbol, " expected (", v_text, ")"});
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

			static bool parse_inner(state & p_state, tokens::nest_tokens & p_tokens, log_pointer p_log, fn_message p_fn) {
				if( t_first::check(p_state) ) {
					typename t_first::t_data v_data;
					if( v_data.parse(p_state, p_tokens, p_log) ) {
						v_data.action(p_state, p_tokens, p_log);
						p_state.m_was_space = std::is_same_v<t_first, t_space>;
						return true;
					}
				}
				if constexpr( s_take_rest ) {
					if( !p_state.m_done ) {
						return t_rest::parse_inner(p_state, p_tokens, p_log, p_fn);
					}
				} else if constexpr( C_none_match_done ) {
					p_log->add({p_state.m_path, p_fn(p_state), p_state.pos()});
					p_state.done();
				}
				return false;
			}

			static bool parse(state & p_state, tokens::nest_tokens & p_tokens, log_pointer p_log) {
				return parse_inner(p_state, p_tokens, p_log, &message);
			}
		};

		//

		template <t_char C_char>
		struct is_char {
			bool parse(state & p_state, tokens::nest_tokens & p_tokens, log_pointer p_log) {
				if( *p_state.m_text_pos == C_char ) {
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

		struct traits {
			static bool take_name(t_raw_const & p_text_pos) {
				if( std::isalpha(*p_text_pos) ) {
					++p_text_pos;
					while( *p_text_pos == '_' || std::isalnum(*p_text_pos) ) { ++p_text_pos; }
					return true;
				}
				return false;
			}

			static bool take_name_with_prefix(state & p_state, t_char p_prefix, t_text_value & p_name, position & p_pos) {
				if( *p_state.m_text_pos != p_prefix ) return false;
				t_raw_const v_name_end = p_state.m_text_pos +1;
				if( take_name(v_name_end) ) {
					p_pos = p_state.pos();
					p_name = just::text_traits::from_range(p_state.m_text_pos, v_name_end);
					p_state.m_text_pos = v_name_end;
					return true;
				}
				return false;
			}
		};

		//

		struct all {

			struct t_type_def_name {
				static t_text_value name() { return "t_type_def_name"_jt; }
				static bool check(state & p_state) { return true; }

				struct t_data {
					// data
					t_text_value	m_name;
					position		m_pos;

					bool parse(state & p_state, tokens::nest_tokens & p_tokens, log_pointer p_log) {
						return traits::take_name_with_prefix(p_state, '$', m_name, m_pos);
					}

					void action(state & p_state, tokens::nest_tokens & p_tokens, log_pointer p_log) {
						p_tokens.m_types.append( new tokens::token_type_add({p_state.m_path, m_pos}, m_name) );
						p_state.m_fn_parse = &rule_type_kind::parse;
					}
				};
			};

			struct t_kw_struct {
				static t_text_value name() { return "t_kw_struct"_jt; }
				static bool check(state & p_state) { return true; }

				struct t_data :
					public is_keyword<"struct">
				{
					void action(state & p_state, tokens::nest_tokens & p_tokens, log_pointer p_log) {
						p_tokens.m_types.append( new tokens::token_struct_add() );
						p_state.m_fn_parse = &rule_struct_open::parse;
					}
				};
			};

			struct t_struct_open {
				static t_text_value name() { return "t_struct_open"_jt; }
				static bool check(state & p_state) { return true; }

				struct t_data :
					public is_char<'('>
				{
					void action(state & p_state, tokens::nest_tokens & p_tokens, log_pointer p_log) {
						p_state.m_fn_parse = &rule_struct_prop::parse;
					}
				};
			};

			struct t_struct_close {
				static t_text_value name() { return "t_struct_close"_jt; }
				static bool check(state & p_state) { return true; }

				struct t_data :
					public is_char<')'>
				{
					void action(state & p_state, tokens::nest_tokens & p_tokens, log_pointer p_log) {
						p_tokens.m_types.append( new tokens::token_struct_end() );
						p_state.m_fn_parse = &rule_decl::parse;
					}
				};
			};

			//

			struct rule_decl :
				public rule_alt<true, t_space, t_type_def_name>
			{};

			struct rule_type_kind :
				public rule_alt<true, t_space, t_kw_struct>
			{};
		
			struct rule_struct_open :
				public rule_alt<true, t_space, t_struct_open>
			{};

			struct rule_struct_prop :
				public rule_alt<true, t_space, t_struct_close>
			{};

		};

	} // ns

} // ns