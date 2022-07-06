module;

#include "../src/pre.h"

export module ksi.rules;

import ksi.tokens;

export namespace ksi {

	namespace rules {

		struct state;

		using t_position = just::t_int_max;
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

		struct state {
			// data
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

			state(t_raw_const p_text_pos, fn_parse p_fn_parse, nest p_nest) :
				m_text_pos{p_text_pos},
				m_line{p_text_pos},
				m_fn_parse{p_fn_parse},
				m_nest{p_nest}
			{}

			void done_nice() {
				m_nice = true;
				m_done = true;
			}

			void done() {
				m_done = true;
			}
		};

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

			static constexpr bool s_take_rest = sizeof...(T_rules) > 1;

			bool parse(state & p_state, tokens::nest_tokens & p_tokens, log_pointer p_log) {
				if( t_first::check(p_state) ) {
					typename t_first::t_data v_data;
					if( v_data.parse(p_state, p_tokens, p_log) ) {
						v_data.action(p_state, p_tokens, p_log);
						return true;
					}
				}
				if constexpr( s_take_rest ) {
					return t_rest::parse(p_state, p_tokens, p_log);
				}
				if constexpr( C_none_match_done ) {
					p_state.done();
				}
				return false;
			}
		};

	} // ns

} // ns