
struct t_literal_null {
	static constexpr kind s_kind{ kind::n_literal };
	static t_text_value name() { return "t_literal_null"_jt; }
	static bool check(state & p_state) { return true; }

	struct t_data :
		public is_keyword<"null">
	{
		void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {
			if( p_state.m_nest != nest::declarative ) { p_tokens.put_literal(var::any_var{}); }
		}
	};
};

struct t_literal_all {
	static constexpr kind s_kind{ kind::n_literal };
	static t_text_value name() { return "t_literal_all"_jt; }
	static bool check(state & p_state) { return true; }

	struct t_data :
		public is_keyword<"all">
	{
		void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {
			if( p_state.m_nest != nest::declarative ) { p_tokens.put_literal(var::variant_all{}); }
		}
	};
};

struct t_literal_false {
	static constexpr kind s_kind{ kind::n_literal };
	static t_text_value name() { return "t_literal_false"_jt; }
	static bool check(state & p_state) { return true; }

	struct t_data :
		public is_keyword<"false">
	{
		void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {
			p_tokens.put_literal(false);
		}
	};
};

struct t_literal_true {
	static constexpr kind s_kind{ kind::n_literal };
	static t_text_value name() { return "t_literal_true"_jt; }
	static bool check(state & p_state) { return true; }

	struct t_data :
		public is_keyword<"true">
	{
		void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {
			p_tokens.put_literal(true);
		}
	};
};

struct t_literal_int {
	static constexpr kind s_kind{ kind::n_literal };
	static t_text_value name() { return "t_literal_int"_jt; }
	static bool check(state & p_state) { return true; }

	struct t_data {
		// data
		t_integer	m_value;

		static bool is_separator(t_char p_char) { return just::is_one_of(p_char, '_', '\''); }

		static bool is_digit_binary(t_char p_char) { return just::is_one_of(p_char, '0', '1'); }
		static bool is_digit_octal(t_char p_char) { return p_char >= '0' && p_char < '8'; }

		static bool is_digit(t_char p_char, int p_radix) {
			switch( p_radix ) {
			case 2: return is_digit_binary(p_char);
			case 8: return is_digit_octal(p_char);
			case 16: return std::isxdigit(p_char);
			}
			return std::isdigit(p_char);
		}

		bool parse(state & p_state, tokens::nest_tokens & p_tokens, log_pointer p_log) {
			t_raw_const v_pos_start = p_state.m_text_pos, v_pos_end;
			std::string_view v_prefix = "0"sv;
			bool v_is_negative = false;
			switch( *v_pos_start ) {
			case '-':
				++v_pos_start;
				v_prefix = "-0"sv;
				v_is_negative = true;
				break;
			case '+':
				++v_pos_start;
				break;
			}
			int v_radix = 10;
			v_pos_end = v_pos_start;
			if( *v_pos_start == '0' ) {
				switch( v_pos_start[1] ) {
				case 'b':
					v_radix = 2;
					v_pos_start += 2;
					if( *v_pos_start == '_' ) { ++v_pos_start; }
					v_pos_end = v_pos_start;
					while( is_digit_binary(*v_pos_end) ) { ++v_pos_end; }
					break;
				case 'o':
					v_radix = 8;
					v_pos_start += 2;
					if( *v_pos_start == '_' ) { ++v_pos_start; }
					v_pos_end = v_pos_start;
					while( is_digit_octal(*v_pos_end) ) { ++v_pos_end; }
					break;
				case 'h':
					v_radix = 16;
					v_pos_start += 2;
					if( *v_pos_start == '_' ) { ++v_pos_start; }
					v_pos_end = v_pos_start;
					while( std::isxdigit(*v_pos_end) ) { ++v_pos_end; }
					break;
				}
			}
			if( v_radix == 10 ) {
				while( std::isdigit(*v_pos_end) ) { ++v_pos_end; }
			}
			if( v_pos_start == v_pos_end && v_radix == 10 ) { return false; }
			t_text_value v_text = just::text_traits::from_range(v_pos_start, v_pos_end);
			v_text = just::implode<t_char>({v_prefix, v_text});
			//
			v_pos_start = v_pos_end;
			while( is_separator(*v_pos_start) && is_digit(v_pos_start[1], v_radix) ) {
				++v_pos_start;
				v_pos_end = v_pos_start +1;
				while( is_digit(*v_pos_end, v_radix) ) { ++v_pos_end; }
				//t_text_value v_text_ending = just::text_traits::from_range(v_pos_start, v_pos_end);
				std::string_view v_text_ending{v_pos_start, static_cast<t_size>(v_pos_end - v_pos_start)};
				v_text = just::implode<t_char>({v_text, v_text_ending});
			}
			errno = 0;
			m_value = std::strtoll(v_text.data(), nullptr, v_radix);
			if( errno == ERANGE ) {
				t_text_value v_message = (v_is_negative ?
					"warning: Integer literal is out of bounds so $int#.max# will be used instead."_jt :
					"warning: Integer literal is out of bounds so $int#.min# will be used instead."_jt
					);
				p_log->add({p_state.m_path, v_message, p_state.pos()});
			}
			p_state.m_text_pos = v_pos_end;
			return true;
		} // fn

		void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {
			//just::g_console << m_value << just::g_new_line;
			p_tokens.put_literal(m_value);
		}
	};
};

struct t_literal_float {
	static constexpr kind s_kind{ kind::n_literal };
	static t_text_value name() { return "t_literal_float"_jt; }
	static bool check(state & p_state) { return true; }

	struct t_data {
		// data
		t_floating	m_value;

		void maybe_separator(t_text_value & p_text, t_raw_const & p_pos_start, t_raw_const & p_pos_end) {
			while( t_literal_int::t_data::is_separator(*p_pos_start) && std::isdigit(p_pos_start[1]) ) {
				++p_pos_start;
				p_pos_end = p_pos_start +1;
				while( std::isdigit(*p_pos_end) ) { ++p_pos_end; }
				//t_text_value v_text_ending = just::text_traits::from_range(p_pos_start, p_pos_end);
				std::string_view v_text_ending{p_pos_start, static_cast<t_size>(p_pos_end - p_pos_start)};
				p_text = just::implode<t_char>({p_text, v_text_ending});
			}
		}

		bool parse(state & p_state, tokens::nest_tokens & p_tokens, log_pointer p_log) {
			t_raw_const v_pos_start = p_state.m_text_pos, v_pos_end;
			//std::string_view v_prefix = ""sv;
			bool v_is_negative = false;
			switch( *v_pos_start ) {
			case '-':
				++v_pos_start;
				//v_prefix = "-"sv;
				v_is_negative = true;
				break;
			case '+':
				++v_pos_start;
				break;
			}
			v_pos_end = v_pos_start;
			while( std::isdigit(*v_pos_end) ) { ++v_pos_end; }
			if( v_pos_end == v_pos_start ) { return false; }
			t_text_value v_text = just::text_traits::from_range(p_state.m_text_pos, v_pos_end);
			bool v_was_dot = false, v_was_e = false;
			v_pos_start = v_pos_end;
			maybe_separator(v_text, v_pos_start, v_pos_end);
			if( *v_pos_end == '.' && std::isdigit(v_pos_end[1]) ) {
				v_was_dot = true;
				v_pos_start = v_pos_end;
				v_pos_end += 2;
				while( std::isdigit(*v_pos_end) ) { ++v_pos_end; }
				std::string_view v_text_ending{v_pos_start, static_cast<t_size>(v_pos_end - v_pos_start)};
				v_text = just::implode<t_char>({v_text, v_text_ending});
			}
			v_pos_start = v_pos_end;
			maybe_separator(v_text, v_pos_start, v_pos_end);
			if( *v_pos_end == 'e' && just::is_one_of(v_pos_end[1], '-', '+') && std::isdigit(v_pos_end[2]) ) {
				v_was_e = true;
				v_pos_start = v_pos_end;
				v_pos_end += 3;
				while( std::isdigit(*v_pos_end) ) { ++v_pos_end; }
				std::string_view v_text_ending{v_pos_start, static_cast<t_size>(v_pos_end - v_pos_start)};
				v_text = just::implode<t_char>({v_text, v_text_ending});
			}
			v_pos_start = v_pos_end;
			maybe_separator(v_text, v_pos_start, v_pos_end);
			if( v_was_e == false && v_was_dot == false ) { return false; }
			errno = 0;
			m_value = std::strtod(v_text.data(), nullptr);
			if( errno == ERANGE ) {
				m_value = std::numeric_limits<t_floating>::infinity();
				t_text_value v_message = "warning: Floating point literal is out of bounds"
				" so $float#.infinity# will be used instead."_jt;
				if( v_is_negative ) {
					m_value = -m_value;
					v_message = "warning: Floating point literal is out of bounds"
					" so $float#.infinity_negative# will be used instead."_jt;
				}
				p_log->add({p_state.m_path, v_message, p_state.pos()});
			}
			p_state.m_text_pos = v_pos_end;
			return true;
		} // fn

		void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {
			//just::g_console << m_value << just::g_new_line;
			p_tokens.put_literal(m_value);
		}
	};
};

struct rule_literal :
	public rule_alt<false,
		t_literal_null,
		t_literal_all,
		t_literal_false,
		t_literal_true,
		t_literal_float,
		t_literal_int
	>
{
	static constexpr kind s_kind{ kind::keep };
	static t_text_value name() { return "rule_literal"_jt; }

	static bool check(state & p_state) {
		return (p_state.m_nest == nest::declarative) ? (p_state.m_kind == kind::n_operator) : true;
	}
};
