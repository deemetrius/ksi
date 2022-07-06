module;

#include "../src/pre.h"

export module ksi.rules;

import ksi.tokens;

export namespace ksi {

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

			position pos(t_raw_const p_text) const {
				return {m_line, p_text - m_line_start +1};
			}
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

			position pos() { return m_line.pos(m_text_pos); }

			void done_nice() {
				m_nice = true;
				m_done = true;
			}

			void done() {
				m_done = true;
			}
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
			using t_rest = first_type<T_rules ...>::template type_rest<C_none_match_done, rule_alt>;
			using t_items = std::vector<t_text_value>;

			static constexpr bool s_take_rest = sizeof...(T_rules) > 1;

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

	} // ns

} // ns