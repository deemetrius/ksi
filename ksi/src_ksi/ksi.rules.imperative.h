
struct t_imp_var_object {
	static constexpr kind s_kind{ kind::special };
	static constexpr just::bits<can> s_can{ };
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
	static constexpr just::bits<can> s_can{ };
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
	static constexpr just::bits<can> s_can{ can_close, can_operator };
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

struct t_imp_operator_common {
	static constexpr kind s_kind{ kind::n_operator };
	static constexpr just::bits<can> s_can{ };
	static t_text_value name() { return "t_imp_operator_common"_jt; }
	static bool check(state & p_state) { return p_state.m_can.has_any(can_operator); }

	struct t_data {
		using operator_info = ast::body::operator_info;

		// data
		position							m_pos;
		typename operator_info::pointer		m_op;

		bool parse(state & p_state, tokens::nest_tokens & p_tokens, log_pointer p_log) {
			for( std::span<operator_info> v_ops = operator_info::ops_common(); operator_info & v_it : v_ops ) {
				if( t_raw_const v_end = just::starts_with<t_char>(p_state.m_text_pos, v_it.m_text) ) {
					m_pos = p_state.pos();
					m_op = &v_it;
					p_state.m_text_pos = v_end;
					return true;
				}
			}
			return false;
		}

		void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {
			p_data->m_late.m_functions.append(
				new tokens::imp_token_add_op(m_pos, m_op)
			);
		}
	};
};

struct t_imp_operator_named {
	static constexpr kind s_kind{ kind::n_operator };
	static constexpr just::bits<can> s_can{ };
	static t_text_value name() { return "t_imp_operator_named"_jt; }
	static bool check(state & p_state) { return p_state.m_can.has_any(can_operator); }

	struct t_data {
		using operator_info = ast::body::operator_info;

		// data
		position							m_pos;
		typename operator_info::pointer		m_op;

		bool parse(state & p_state, tokens::nest_tokens & p_tokens, log_pointer p_log) {
			t_raw_const v_end = p_state.m_text_pos;
			if( traits<false>::impl_take_name(v_end) ) {
				t_text_value v_name = just::text_traits::from_range(p_state.m_text_pos, v_end);
				typename operator_info::t_map & v_map = operator_info::ops_named();
				typename operator_info::t_map::iterator v_it = v_map.find(v_name);
				if( v_it == v_map.end() ) { return false; }
				m_pos = p_state.pos();
				m_op = &(*v_it).second;
				p_state.m_text_pos = v_end;
				return true;
			}
			return false;
		}

		void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {
			p_data->m_late.m_functions.append(
				new tokens::imp_token_add_op(m_pos, m_op)
			);
		}
	};
};
