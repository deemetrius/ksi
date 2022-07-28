
struct t_type_def_name {
	static constexpr kind s_kind{ kind::special };
	static t_text_value name() { return "t_type_def_name"_jt; }
	static bool check(state & p_state) { return true; }

	struct t_data {
		// data
		t_text_value	m_name;
		position		m_pos;
		bool			m_is_local = false;

		bool parse(state & p_state, tokens::nest_tokens & p_tokens, log_pointer p_log) {
			bool ret = traits<>::take_name_with_prefix(p_state, '$', m_name, m_pos);
			if( ret && *p_state.m_text_pos == '@' ) {
				++p_state.m_text_pos;
				m_is_local = true;
			}
			return ret;
		}

		void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {
			p_tokens.m_types.append(
				new tokens::token_type_add({m_name, m_is_local, {p_state.m_path, m_pos}})
			);
			p_state.m_fn_parse = &rule_type_kind::parse;
		}
	};
};

// refers

struct t_kw_refers {
	static constexpr kind s_kind{ kind::special };
	static t_text_value name() { return "t_kw_refers"_jt; }
	static bool check(state & p_state) { return ! p_state.flag_check_any(flag_was_refers | flag_was_extends); }

	struct t_data :
		public is_keyword<"refers">
	{
		void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {
			p_state.flag_set(flag_was_refers);
			p_state.m_fn_parse = &rule_refers_open::parse;
		}
	};
};

struct t_refers_open {
	static constexpr kind s_kind{ kind::start };
	static t_text_value name() { return "t_refers_open"_jt; }
	static bool check(state & p_state) { return true; }

	struct t_data :
		public is_char<'('>
	{
		void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {
			p_state.m_fn_parse = &rule_refers_inside::parse;
		}
	};
};

struct t_refers_close {
	static constexpr kind s_kind{ kind::special };
	static t_text_value name() { return "t_refers_close"_jt; }
	static bool check(state & p_state) { return p_state.m_kind != kind::start; }

	struct t_data :
		public is_char<')'>
	{
		void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {
			p_state.m_fn_parse = &rule_type_kind::parse;
		}
	};
};

struct t_refers_category_name {
	static constexpr kind s_kind{ kind::special };
	static t_text_value name() { return "t_refers_category_name"_jt; }
	static bool check(state & p_state) { return true; }

	struct t_data :
		public is_category<false>
	{
		void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {
			p_tokens.m_types.append( new tokens::token_type_add_category(m_entity) );
		}
	};
};

// extends

struct t_kw_extends {
	static constexpr kind s_kind{ kind::special };
	static t_text_value name() { return "t_kw_extends"_jt; }
	static bool check(state & p_state) { return ! p_state.flag_check(flag_was_extends); }

	struct t_data :
		public is_keyword<"extends">
	{
		void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {
			p_state.m_fn_parse = &rule_extends_open::parse;
			p_state.flag_set(flag_was_extends);
		}
	};
};

struct t_extends_open {
	static constexpr kind s_kind{ kind::start };
	static t_text_value name() { return "t_extends_open"_jt; }
	static bool check(state & p_state) { return true; }

	struct t_data :
		public is_char<'('>
	{
		void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {
			p_state.m_fn_parse = &rule_extends_inside::parse;
		}
	};
};

struct t_extends_close {
	static constexpr kind s_kind{ kind::special };
	static t_text_value name() { return "t_extends_close"_jt; }
	static bool check(state & p_state) { return p_state.m_kind != kind::start; }

	struct t_data :
		public is_char<')'>
	{
		void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {
			p_state.m_fn_parse = &rule_type_kind::parse;
		}
	};
};

struct t_extends_type_name {
	static constexpr kind s_kind{ kind::special };
	static t_text_value name() { return "t_extends_type_name"_jt; }
	static bool check(state & p_state) { return true; }

	struct t_data :
		public is_type<false>
	{
		void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {
			p_tokens.m_types.append( new tokens::token_type_add_base(m_entity) );
		}
	};
};

// struct

struct t_kw_struct {
	static constexpr kind s_kind{ kind::special };
	static t_text_value name() { return "t_kw_struct"_jt; }
	static bool check(state & p_state) { return true; }

	struct t_data :
		public is_keyword<"struct">
	{
		void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {
			p_tokens.m_types.append(new tokens::token_struct_add{p_state.m_path});
			p_state.m_fn_parse = &rule_struct_open::parse;
		}
	};
};

struct t_struct_open {
	static constexpr kind s_kind{ kind::start };
	static t_text_value name() { return "t_struct_open"_jt; }
	static bool check(state & p_state) { return true; }

	struct t_data :
		public is_char<'('>
	{
		void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {
			p_state.m_fn_parse = &rule_struct_inside::parse;
		}
	};
};

struct t_struct_close {
	static constexpr kind s_kind{ kind::special };
	static t_text_value name() { return "t_struct_close"_jt; }
	static bool check(state & p_state) { return p_state.m_kind != kind::n_operator; }

	struct t_data :
		public is_char<')'>
	{
		void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {
			p_tokens.m_types.append( new tokens::token_struct_end() );
			p_state.m_fn_parse = &rule_decl::parse;
			p_state.flag_unset(flag_was_refers | flag_was_extends);
		}
	};
};

struct t_struct_prop_name {
	static constexpr kind s_kind{ kind::variable };
	static t_text_value name() { return "t_struct_prop_name"_jt; }
	static bool check(state & p_state) {
		if( p_state.m_kind == kind::n_literal && (p_state.m_was_space == false) ) { return false; }
		return p_state.m_kind != kind::n_operator;
	}

	struct t_data
	{
		// data
		position		m_pos;
		t_text_value	m_name;

		bool parse(state & p_state, tokens::nest_tokens & p_tokens, log_pointer p_log) {
			return traits<>::take_name(p_state, m_name, m_pos);
		}

		void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {
			p_tokens.m_types.append( new tokens::token_struct_prop_name({p_state.m_path, m_pos}, m_name) );
		}
	};
};

struct t_struct_prop_separator {
	static constexpr kind s_kind{ kind::separator };
	static t_text_value name() { return "t_struct_prop_separator"_jt; }
	static bool check(state & p_state) {
		return ! just::is_one_of(p_state.m_kind, kind::start, kind::n_operator, kind::separator);
	}

	struct t_data :
		public is_char<',', ';'>
	{
		void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {}
	};
};

struct t_struct_prop_assign {
	static constexpr kind s_kind{ kind::n_operator };
	static t_text_value name() { return "t_struct_prop_assign"_jt; }
	static bool check(state & p_state) {
		return p_state.m_kind == kind::variable;
	}

	struct t_data :
		public is_char<'='>
	{
		void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {}
	};
};
