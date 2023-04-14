#pragma once

struct t_module_def {
	static inline t_text s_name = L"t_module_def"s;
	static constexpr kind s_kind = kind::n_special;

	static bool check(state & p_state) { return true; }

	struct t_data : public is_prefix_name<L'@'> {
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
	static inline t_text s_name = L"t_var_mod"s;
	static constexpr kind s_kind = kind::n_variable;

	static bool check(state & p_state) { return true; }

	struct t_data : public is_name {
		void action(state & p_state, tokens::nest & p_tokens, ast::prepare_data & p_data) {
			p_tokens.add(this);
			p_state.m_next_parse = &rule_module_after_var::parse;
		}

		void rule_perform(ast::prepare_data & p_data, tokens::token_info & p_info) {
			//t_index v_var_id = p_data.var_add(m_name, m_path, m_pos);
			//
		}
	};
};

struct t_var_mod_assign {
	static inline t_text s_name = L"t_var_mod_assign"s;
	static constexpr kind s_kind = kind::n_operator;

	static bool check(state & p_state) { return true; }

	struct t_data : public is_char<L'='> {
		void action(state & p_state, tokens::nest & p_tokens, ast::prepare_data & p_data) {
			p_state.m_next_parse = &rule_module_after_assign::parse;
		}

		void rule_perform(ast::prepare_data & p_data, tokens::token_info & p_info) {
		}
	};
};

struct end_var_mod_assign {
	void action(state & p_state, tokens::nest & p_tokens, ast::prepare_data & p_data) {
		p_tokens.add(this);
	}

	void rule_perform(ast::prepare_data & p_data, tokens::token_info & p_info) {
		// todo: apply ast tree to seq
	}
};

struct t_literal_int {
	static inline t_text s_name = L"t_literal_int"s;
	static constexpr kind s_kind = kind::n_literal;

	static bool check(state & p_state) {
		if( p_state.m_nest == nest_type::n_module && p_state.m_was_kind == kind::n_literal ) {
			return false;
		}
		return true;
	}

	struct t_data : public is_integer {
		void action(state & p_state, tokens::nest & p_tokens, ast::prepare_data & p_data) {
		}

		void rule_perform(ast::prepare_data & p_data, tokens::token_info & p_info) {
		}
	};
};
