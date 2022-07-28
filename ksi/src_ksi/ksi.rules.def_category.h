
struct t_category_def_name {
	static constexpr kind s_kind{ kind::special };
	static t_text_value name() { return "t_category_def_name"_jt; }
	static bool check(state & p_state) { return true; }

	struct t_data {
		// data
		t_text_value	m_name;
		position		m_pos;
		bool			m_is_local = false;

		bool parse(state & p_state, tokens::nest_tokens & p_tokens, log_pointer p_log) {
			bool ret = traits<>::take_name_with_prefix(p_state, '_', m_name, m_pos);
			if( ret && *p_state.m_text_pos == '@' ) {
				++p_state.m_text_pos;
				m_is_local = true;
			}
			return ret;
		}

		void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {
			p_tokens.m_cats.append(
				new tokens::token_category_add({m_name, m_is_local, {p_state.m_path, m_pos}})
			);
			p_state.m_fn_parse = &rule_category_open::parse;
		}
	};
};

struct t_category_open {
	static constexpr kind s_kind{ kind::start };
	static t_text_value name() { return "t_category_open"_jt; }
	static bool check(state & p_state) { return true; }

	struct t_data :
		public is_char<'('>
	{
		void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {
			p_state.m_fn_parse = &rule_category_inside::parse;
		}
	};
};

struct t_category_close {
	static constexpr kind s_kind{ kind::special };
	static t_text_value name() { return "t_category_close"_jt; }
	static bool check(state & p_state) { return true; }

	struct t_data :
		public is_char<')'>
	{
		void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {
			p_state.m_fn_parse = &rule_decl::parse;
		}
	};
};

struct t_category_includes_name {
	static constexpr kind s_kind{ kind::special };
	static t_text_value name() { return "t_category_includes_name"_jt; }
	static bool check(state & p_state) { return true; }

	struct t_data :
		is_category<false>
	{
		void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {
			p_tokens.m_cats.append( new tokens::token_category_add_base(p_state.m_path, m_entity) );
		}
	};
};
