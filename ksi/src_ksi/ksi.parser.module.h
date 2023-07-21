#pragma once

struct t_module_def {
	static inline constexpr text_view
		s_name = L"t_module_def"sv;
	static constexpr kind
		s_kind = kind::n_special;

	static bool check(state & p_state) { return true; }

	struct t_data : public is_name<g_module_char> {
		void action(state & p_state, tokens::nest & p_tokens, ast::prepare_data & p_data) {
			p_tokens.add(this);
			p_state.m_nest = nest_type::n_module;
			p_state.m_next_parse = &rule_module::parse;
		}

		void rule_perform(ast::prepare_data & p_data, tokens::token_info & p_info) {
			p_data.m_mod_current = p_data.mod_get(this->m_name);
		}
	};
};

struct t_var_mod {
	static inline constexpr text_view
		s_name = L"t_var_mod"sv;
	static constexpr kind
		s_kind = kind::n_variable;

	static bool check(state & p_state) { return true; }

	struct t_data : public is_name<g_null_char, g_module_char> {
		void action(state & p_state, tokens::nest & p_tokens, ast::prepare_data & p_data) {
			p_tokens.add(this);
			p_state.m_next_parse = &rule_module_after_var::parse;
		}

		void rule_perform(ast::prepare_data & p_data, tokens::token_info & p_info) {
			t_text v_full_name = p_data.var_full_name(m_name);
			t_index v_var_id = p_data.var_add(v_full_name, m_path, m_pos);
			p_data.m_var_pos.m_module_id = p_data.m_mod_current->m_module->id();
			p_data.m_var_pos.m_aspect_id = v_var_id;
			act::action::pointer v_action = p_data.m_action_pool->make(
				&act::actions::s_mod_var_link,
				act::action_data{m_pos, 0, p_data.m_var_pos}
			);
			p_data.m_body->node_add(&ast::body::node_types::s_leaf, v_action);
		}
	};
};

struct t_var_mod_assign {
	static inline constexpr text_view
		s_name = L"t_var_mod_assign"sv;
	static constexpr kind
		s_kind = kind::n_operator;

	static bool check(state & p_state) { return true; }

	struct t_data : public is_char<L'='> {
		void action(state & p_state, tokens::nest & p_tokens, ast::prepare_data & p_data) {
			p_state.m_next_parse = &rule_module_after_assign::parse;
			p_tokens.add(this);
		}

		void rule_perform(ast::prepare_data & p_data, tokens::token_info & p_info) {
			act::action::pointer v_action = p_data.m_action_pool->make(
				&act::actions::s_assign,
				act::action_data{m_pos}
			);
			p_data.m_body->node_add(&ast::body::node_types::s_assign, v_action);
		}
	};
};

struct end_var_mod_assign {
	// data
	t_pos
		m_space_pos;

	void action(state & p_state, tokens::nest & p_tokens, ast::prepare_data & p_data) {
		m_space_pos = p_state.m_space_pos;
		p_tokens.add(this);
	}

	void rule_perform(ast::prepare_data & p_data, tokens::token_info & p_info) {
		//
		{
			ast::body::body_pointer v_body = p_data.m_body.get();
			v_body->apply();
			act::action::pointer v_action = p_data.m_action_pool->make(
				&act::actions::s_mod_var_ready,
				act::action_data{m_space_pos, 0, p_data.m_var_pos}
			);
			v_body->action_add(v_action);
		}
		p_data.m_body.reset();
	}
};

struct t_literal_int {
	static inline constexpr text_view
		s_name = L"t_literal_int"sv;
	static constexpr kind
		s_kind = kind::n_literal;

	static bool check(state & p_state) {
		if( p_state.m_nest == nest_type::n_module && p_state.m_was_kind == kind::n_literal ) {
			return false;
		}
		return true;
	}

	struct t_data : public is_integer {
		void action(state & p_state, tokens::nest & p_tokens, ast::prepare_data & p_data) {
			p_tokens.add(this);
		}

		void rule_perform(ast::prepare_data & p_data, tokens::token_info & p_info) {
			act::action::pointer v_action = p_data.m_action_pool->make(
				&act::actions::s_put_int,
				act::action_data{m_pos, m_value}
			);
			p_data.m_body->node_add(&ast::body::node_types::s_leaf, v_action);
		}
	};
};
