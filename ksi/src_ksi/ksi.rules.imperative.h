
struct t_imp_var_object {
	static constexpr kind s_kind{ kind::special };
	static constexpr flags_raw s_can{ 0 };
	static t_text_value name() { return "t_imp_var_object"_jt; }
	static bool check(state & p_state) { return true; }

	struct t_data :
		public is_keyword<"v">
	{
		void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {
			p_state.m_fn_parse = &rule_imp_var_dot::parse;
		}
	};
};

struct t_imp_var_object_dot {
	static constexpr kind s_kind{ kind::special };
	static constexpr flags_raw s_can{ 0 };
	static t_text_value name() { return "t_imp_var_object_dot"_jt; }
	static bool check(state & p_state) { return true; }

	struct t_data :
		public is_char<'.'>
	{
		void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {
			p_state.m_fn_parse = &rule_imp_var_name::parse;
		}
	};
};

struct t_imp_var_name {
	static constexpr kind s_kind{ kind::variable };
	static constexpr flags_raw s_can{ can_close };
	static t_text_value name() { return "t_imp_var_name"_jt; }
	static bool check(state & p_state) { return true; }

	struct t_data {
		// data
		t_text_value	m_name;

		bool parse(state & p_state, tokens::nest_tokens & p_tokens, log_pointer p_log) {
			position v_pos;
			return traits<false>::take_name(p_state, m_name, v_pos);
		}

		void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {
			p_data->m_late.m_functions.append(
				new tokens::imp_token_put_var(tokens::how_put::copy, m_name)
			);
			p_state.m_fn_parse = &rule_function_inside::parse;
		}
	};
};
