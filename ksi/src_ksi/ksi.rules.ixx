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
				m_tab_extra = 0;
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
			imperative,
			fn_body
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

		enum can : flags_raw {
			can_keep			= 1 << 0,
			can_close			= 1 << 1,
			can_dot_param		= 1 << 2,
			can_dot_var			= 1 << 3,
		};

		enum flags : flags_raw {
			flag_allow_plain	= 1 << 0,
			flag_was_refers		= 1 << 1,
			flag_was_extends	= 1 << 2,
			flag_was_fn_params	= 1 << 3,
			flag_was_colon		= 1 << 4,
		};

		//

		struct state :
			public state_base
		{
			using t_nest = std::vector<nest>;

			// data
			fs::path		m_path;
			t_raw_const		m_text_pos;
			fn_parse		m_fn_parse;
			t_nest			m_nest;
			kind			m_kind = kind::start;
			flags_raw		m_flags = 0;
			flags_raw		m_can = 0;
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
				m_nest{1, p_nest}
			{}

			position pos() const { return m_line.pos(m_text_pos); }

			nest nest_last() { return m_nest.back(); }
			void nest_add(nest p_nest) { m_nest.push_back(p_nest); }
			void nest_del() { m_nest.pop_back(); }

			bool can_check(flags_raw p_flag) { return (m_can & p_flag) == p_flag; }
			bool can_check_any(flags_raw p_flag) { return m_can & p_flag; }

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

		// basic rules
		#include "ksi.rules.basic.h"

		struct all {

			struct t_module_name {
				static constexpr kind s_kind{ kind::special };
				static constexpr flags_raw s_can{ 0 };
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

			// def category
			#include "ksi.rules.def_category.h"

			// def struct
			#include "ksi.rules.def_struct.h"

			// def function
			#include "ksi.rules.def_function.h"

			// literals
			#include "ksi.rules.literals.h"

			// rules

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
					t_separator
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
				public rule_alt<true, t_space, t_function_params_open, t_function_open> {};

			struct rule_function_params_inside :
				public rule_alt<true, t_space,
					t_function_params_close,
					t_function_param_name,
					t_function_param_assign,
					rule_literal,
					t_separator
				>
			{};

			struct rule_function_inside :
				public rule_alt<true, t_space,
					t_function_close,
					rule_literal,
					t_separator
				>
			{};

		}; // struct all

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