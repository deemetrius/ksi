
struct t_eof {
	static constexpr kind s_kind{ kind::end };
	static t_text_value name() { return "t_eof"_jt; }
	static bool check(state & p_state) { return true; }

	struct t_data {
		bool parse(state & p_state, tokens::nest_tokens & p_tokens, log_pointer p_log) {
			return *p_state.m_text_pos == '\0';
		}

		void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {
			p_state.done_nice();
		}
	};
};

struct t_space {
	static constexpr kind s_kind{ kind::space };
	static t_text_value name() { return "t_space"_jt; }
	static bool check(state & p_state) { return !p_state.m_was_space; }

	enum class nest_comments { none, line, multiline };

	struct t_data {
		static bool take_new_line(state & p_state, t_raw_const & p_text_pos, bool & p_continue) {
			switch( *p_text_pos ) {
			case '\n':
				p_continue = true;
				++p_text_pos;
				p_state.m_line.next_line(p_text_pos);
				return true;
			case '\r':
				p_continue = true;
				if( p_text_pos[1] == '\n' ) { p_text_pos += 2; }
				else { ++p_text_pos; }
				p_state.m_line.next_line(p_text_pos);
				return true;
			}
			return false;
		}

		static bool take_multiline_comment_open(t_raw_const & p_text_pos, t_index & p_depth, bool & p_continue) {
			if( *p_text_pos == '/' && p_text_pos[1] == '*' ) {
				p_continue = true;
				++p_depth;
				p_text_pos += 2;
				return true;
			}
			return false;
		}

		bool parse(state & p_state, tokens::nest_tokens & p_tokens, log_pointer p_log) {
			t_raw_const v_text_pos = p_state.m_text_pos;
			//
			{
				bool v_continue;
				nest_comments v_nest = nest_comments::none;
				t_index v_depth = 0;
				do {
					v_continue = false;
					if( *v_text_pos == '\0' ) break;
					bool v_new_line = take_new_line(p_state, v_text_pos, v_continue);
					switch( v_nest ) {
					case nest_comments::none :
						if( take_multiline_comment_open(v_text_pos, v_depth, v_continue) ) {
							v_nest = nest_comments::multiline;
						} else {
							switch( *v_text_pos ) {
							case '\t':
								p_state.tab(v_text_pos);
							case ' ':
								v_continue = true;
								++v_text_pos;
								break;
							case '-':
								if( v_text_pos[1] == '-' ) {
									v_continue = true;
									v_text_pos += 2;
									v_nest = nest_comments::line;
								}
								break;
							}
						}
						break;
					case nest_comments::line :
						if( v_new_line ) { v_nest = nest_comments::none; break; }
						v_continue = true;
						++v_text_pos;
						break;
					case nest_comments::multiline :
						take_multiline_comment_open(v_text_pos, v_depth, v_continue);
						if( *v_text_pos == '*' && v_text_pos[1] == '/' ) {
							v_continue = true;
							v_text_pos += 2;
							--v_depth;
							if( v_depth == 0 ) { v_nest = nest_comments::none; }
						} else {
							v_continue = true;
							++v_text_pos;
						}
						break;
					}
				} while( v_continue );
			}
			if( p_state.m_text_pos != v_text_pos ) {
				p_state.m_text_pos = v_text_pos;
				return true;
			}
			return false;
		}

		void action(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {}
	};
};

//

template <typename, typename = void>
struct is_rule_inner : public std::false_type {};

template <typename T>
struct is_rule_inner<T, std::void_t<typename T::t_data> > : public std::true_type {};

template <typename T>
constexpr bool is_rule_inner_v = is_rule_inner<T>::value;

//

template <typename ... T_items>
struct first_type;

template <typename T_first, typename ... T_rest>
struct first_type<T_first, T_rest ...> {
	using type = T_first;

	template <bool C_flag, template <bool, typename ...> typename T_template>
	using type_rest = T_template<C_flag, T_rest ...>;
};

template <typename ... T_items>
using first_type_t = first_type<T_items ...>::type;

template <bool C_none_match_done, typename ... T_rules>
struct rule_alt {
	using t_first = first_type_t<T_rules ...>;
	using t_items = std::vector<t_text_value>;

	static constexpr bool s_take_rest = sizeof...(T_rules) > 1;

	using t_rest = std::conditional_t<s_take_rest,
		typename first_type<T_rules ...>::template type_rest<C_none_match_done, rule_alt>,
		void
	>;

	static t_text_value message(state & p_state) {
		t_items v_items;
		add_items(p_state, v_items);
		t_text_value v_text = just::implode_items<t_char, t_text_value>(v_items, ", "sv);
		std::string_view v_symbol = "symbol";
		if( *p_state.m_text_pos == '\0' ) { v_symbol = "EOF"; }
		return just::implode<t_char>({"parse error: Unexpected "sv, v_symbol, ". Expected ("sv, v_text, ")"sv});
	}

	using fn_message = decltype(&message);

	static void add_items(state & p_state, t_items & p_items) {
		if( t_first::check(p_state) ) {
			p_items.push_back(t_first::name() );
		}
		if constexpr( s_take_rest ) {
			t_rest::add_items(p_state, p_items);
		}
	}

	static bool parse_inner(state & p_state, tokens::nest_tokens & p_tokens,
		prepare_data::pointer p_data, fn_message p_fn
	) {
		if( t_first::check(p_state) ) {
			bool v_nice = false;
			if constexpr( is_rule_inner_v<t_first> ) {
				typename t_first::t_data v_data;
				if( v_data.parse(p_state, p_tokens, p_data->m_log) ) {
					v_data.action(p_state, p_tokens, p_data);
					v_nice = true;
				}
			} else if( t_first::parse(p_state, p_tokens, p_data) ) {
				v_nice = true;
			}
			if( v_nice ) {
				p_state.m_was_space = std::is_same_v<t_first, t_space>;
				if constexpr( ! just::is_one_of(t_first::s_kind, kind::space, kind::keep) ) {
					p_state.m_kind = t_first::s_kind;
				}
				return true;
			}
		}
		if constexpr( s_take_rest ) {
			if( !p_state.m_done ) {
				return t_rest::parse_inner(p_state, p_tokens, p_data, p_fn);
			}
		} else if constexpr( C_none_match_done ) {
			p_data->m_log->add({p_state.m_path, p_fn(p_state), p_state.pos()});
			p_state.done();
		}
		return false;
	}

	static bool parse(state & p_state, tokens::nest_tokens & p_tokens, prepare_data::pointer p_data) {
		return parse_inner(p_state, p_tokens, p_data, &message);
	}
};

//

template <t_char ... C_char>
struct is_char {
	bool parse(state & p_state, tokens::nest_tokens & p_tokens, log_pointer p_log) {
		if( just::is_one_of(*p_state.m_text_pos, C_char ...) ) {
			++p_state.m_text_pos;
			return true;
		}
		return false;
	}
};

template <just::fixed_string C_text>
struct is_keyword {
	bool parse(state & p_state, tokens::nest_tokens & p_tokens, log_pointer p_log) {
		if( just::text_traits::cmp_n(p_state.m_text_pos, C_text.m_text, C_text.s_length) == 0 ) {
			t_char v_after = p_state.m_text_pos[C_text.s_length];
			if( std::isalnum(v_after) || v_after == '_' ) { return false; }
			p_state.m_text_pos += C_text.s_length;
			return true;
		}
		return false;
	}
};

template <bool C_allow_std = false>
struct traits {
	static bool impl_take_name(t_raw_const & p_text_pos) {
		if( std::isalpha(*p_text_pos) ) {
			++p_text_pos;
			while( *p_text_pos == '_' || std::isalnum(*p_text_pos) ) { ++p_text_pos; }
			if constexpr( C_allow_std ) {
				if( *p_text_pos == '#' ) { ++p_text_pos; }
			}
			return true;
		}
		return false;
	}

	static bool take_name(state & p_state, t_text_value & p_name, position & p_pos) {
		t_raw_const v_name_end = p_state.m_text_pos;
		if( impl_take_name(v_name_end) ) {
			p_pos = p_state.pos();
			p_name = just::text_traits::from_range(p_state.m_text_pos, v_name_end);
			p_state.m_text_pos = v_name_end;
			return true;
		}
		return false;
	}

	static bool take_name_with_prefix(state & p_state, t_char p_prefix, t_text_value & p_name, position & p_pos) {
		if( *p_state.m_text_pos != p_prefix ) return false;
		t_raw_const v_name_end = p_state.m_text_pos +1;
		if( impl_take_name(v_name_end) ) {
			p_pos = p_state.pos();
			p_name = just::text_traits::from_range(p_state.m_text_pos, v_name_end);
			p_state.m_text_pos = v_name_end;
			return true;
		}
		return false;
	}
};

template <bool C_allow_std, t_char T_prefix>
struct is_entity
{
	// data
	entity_info		m_entity;

	bool parse(state & p_state, tokens::nest_tokens & p_tokens, log_pointer p_log) {
		bool ret = traits<C_allow_std>::take_name_with_prefix(p_state, T_prefix, m_entity.m_name, m_entity.m_pos);
		if( ret ) {
			position v_pos;
			if( traits<C_allow_std>::take_name_with_prefix(p_state, '@', m_entity.m_module_name, v_pos) ) {}
			else if( *p_state.m_text_pos == '@' ) {
				++p_state.m_text_pos;
				m_entity.m_module_name = "@"_jt;
			}
		}
		return ret;
	}
};

template <bool C_allow_std>
struct is_category :
	public is_entity<C_allow_std, '_'>
{};

template <bool C_allow_std>
struct is_type :
	public is_entity<C_allow_std, '$'>
{};
