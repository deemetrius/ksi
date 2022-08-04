
// fn_def: fn_name overload arg_name params fn_body
// fn_name: &name | &name@
// overload: [ _category | $type ]
// params: [ '{' repeat( param_name [ = literal] [',' | ';'] ) '}' ]
// fn_body: '(' [ expressions ] ')'

struct t_function_def_name {
	static constexpr kind s_kind{ kind::start };
	static constexpr flags_raw s_can{ 0 };
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
	static constexpr flags_raw s_can{ 0 };
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
	static constexpr flags_raw s_can{ 0 };
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
	static constexpr flags_raw s_can{ 0 };
	static t_text_value name() { return "t_function_arg"_jt; }
	static bool check(state & p_state) { return true; }

	struct t_data {
		// data
		t_text_value	m_name;

		bool parse(state & p_state, tokens::nest_tokens & p_tokens, log_pointer p_log) {
			position v_pos;
			return traits<false>::take_name(p_state, m_name, v_pos);
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

struct t_function_params_open {
	static constexpr kind s_kind{ kind::start };
	static constexpr flags_raw s_can{ 0 };
	static t_text_value name() { return "t_function_params_open"_jt; }
	static bool check(state & p_state) { return ! p_state.flag_check_any(flag_was_fn_params); }

	struct t_data :
		public is_char<'{'>
	{
		void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {
			p_state.m_fn_parse = &rule_function_params_inside::parse;
			p_state.flag_set(flag_was_fn_params);
			p_tokens.m_fn_put_literal = &tokens::nest_tokens::put_literal_fn_param_default;
		}
	};
};

struct t_function_params_close {
	static constexpr kind s_kind{ kind::special };
	static constexpr flags_raw s_can{ 0 };
	static t_text_value name() { return "t_function_params_close"_jt; }
	static bool check(state & p_state) { return ! just::is_one_of(p_state.m_kind, kind::start, kind::n_operator); }

	struct t_data :
		public is_char<'}'>
	{
		void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {
			p_state.m_fn_parse = &rule_function_open::parse;
		}
	};
};

struct t_function_param_name {
	static constexpr kind s_kind{ kind::variable };
	static constexpr flags_raw s_can{ 0 };
	static t_text_value name() { return "t_function_param_name"_jt; }
	static bool check(state & p_state) { return true; }

	struct t_data {
		// data
		position		m_pos;
		t_text_value	m_name;

		bool parse(state & p_state, tokens::nest_tokens & p_tokens, log_pointer p_log) {
			return traits<false>::take_name(p_state, m_name, m_pos);
		}

		void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {
			p_data->m_late.m_functions.append(
				new tokens::late_token_function_add_param({p_state.m_path, m_pos}, m_name)
			);
		}
	};
};

struct t_function_param_separator {
	static constexpr kind s_kind{ kind::separator };
	static constexpr flags_raw s_can{ 0 };
	static t_text_value name() { return "t_function_param_separator"_jt; }
	static bool check(state & p_state) {
		return ! just::is_one_of(p_state.m_kind, kind::start, kind::n_operator, kind::separator);
	}

	struct t_data :
		public is_char<',', ';'>
	{
		void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {}
	};
};

struct t_function_param_assign {
	static constexpr kind s_kind{ kind::n_operator };
	static constexpr flags_raw s_can{ 0 };
	static t_text_value name() { return "t_function_param_assign"_jt; }
	static bool check(state & p_state) {
		return p_state.m_kind == kind::variable;
	}

	struct t_data :
		public is_char<'='>
	{
		void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {}
	};
};

struct t_function_open {
	static constexpr kind s_kind{ kind::start };
	static constexpr flags_raw s_can{ 0 };
	static t_text_value name() { return "t_function_open"_jt; }
	static bool check(state & p_state) { return true; }

	struct t_data :
		public is_char<'('>
	{
		void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {
			p_state.m_fn_parse = &rule_function_inside::parse;
			p_tokens.m_fn_put_literal = &tokens::nest_tokens::put_literal_imperative;
			p_state.nest_add(nest::fn_body);
		}
	};
};

struct t_function_close {
	static constexpr kind s_kind{ kind::special };
	static constexpr flags_raw s_can{ 0 };
	static t_text_value name() { return "t_function_close"_jt; }
	static bool check(state & p_state) { return true; }

	struct t_data :
		public is_char<')'>
	{
		void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {
			p_data->m_late.m_functions.append(
				new tokens::late_token_function_body_close()
			);
			p_state.m_fn_parse = &rule_decl::parse;
			p_state.flag_unset(flag_was_fn_params);
			p_state.nest_del();
		}
	};
};
