
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
