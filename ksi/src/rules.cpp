#include "rules.ksi.h"

namespace ksi {
namespace rules {

void rule_start::parse(const ex::wtext::Char * str, info & inf, t_tokens & toks, base_log * log) {
	state st = {
		&inf,
		str,
		str,
		{str},
		0,
		hive::rule_file::parse
	};
	base_rule::go(st, toks, log);
}

//

void for_dot_get::add_bracket_end(state & st, t_tokens & toks) {
	toks.append( new tokens::token_bracket_end() );
}

//

void hive::keyword_plain::post_action(state & st, t_tokens & toks, base_log * log) {
	toks.append(new tokens::token_plain(st.liner_.get_pos(st.prev_str_)) );
	st.next_fn_ = rule_expr::parse;
}

void end_plain::post_action(state & st, t_tokens & toks, base_log * log) {
	toks.append(new tokens::token_fn_end() );
	pa_done_good::post_action(st, toks, log);
}

bool hive::operand_scope::parse(state & st, t_tokens & toks, base_log * log) {
	if( *st.str_ != L'(' ) return false;
	hive::operand::maybe_next_expr(st, toks, log);
	toks.append( new tokens::token_scope_begin(st.liner_.get_pos(st.str_) ) );
	const Char * str = st.str_ +1;
	info tmp_inf = { st.inf_->path_ };
	state tmp_st = {
		&tmp_inf,
		str,
		str,
		st.liner_,
		st.loop_depth_,
		rule_expr::parse,
		nest_parentheses
	};
	base_rule::go(tmp_st, toks, log);
	bool ret = false;
	if( tmp_inf.good_ ) {
		toks.append( new tokens::token_scope_end() );
		st.next_str(tmp_st.str_);
		st.liner_ = tmp_st.liner_;
		ret = true;
	} else {
		st.done_ = true;
	}
	return ret;
}

bool hive::operand_array::parse(state & st, t_tokens & toks, base_log * log) {
	if( *st.str_ != L'[' ) return false;
	hive::operand::maybe_next_expr(st, toks, log);
	toks.append( new tokens::token_array_begin(st.liner_.get_pos(st.str_) ) );
	const Char * str = st.str_ +1;
	info tmp_inf = { st.inf_->path_ };
	state tmp_st = {
		&tmp_inf,
		str,
		str,
		st.liner_,
		st.loop_depth_,
		rule_expr::parse,
		nest_array
	};
	base_rule::go(tmp_st, toks, log);
	bool ret = false;
	if( tmp_inf.good_ ) {
		toks.append( new tokens::token_array_end() );
		st.next_str(tmp_st.str_);
		st.liner_ = tmp_st.liner_;
		ret = true;
	} else {
		st.done_ = true;
	}
	return ret;
}

bool hive::operand_map::parse(state & st, t_tokens & toks, base_log * log) {
	if( *st.str_ != L'{' ) return false;
	hive::operand::maybe_next_expr(st, toks, log);
	toks.append( new tokens::token_map_begin(st.liner_.get_pos(st.str_) ) );
	const Char * str = st.str_ +1;
	info tmp_inf = { st.inf_->path_ };
	state tmp_st = {
		&tmp_inf,
		str,
		str,
		st.liner_,
		st.loop_depth_,
		rule_expr::parse,
		nest_map
	};
	base_rule::go(tmp_st, toks, log);
	bool ret = false;
	if( tmp_inf.good_ ) {
		toks.append( new tokens::token_map_end() );
		st.next_str(tmp_st.str_);
		st.liner_ = tmp_st.liner_;
		ret = true;
	} else {
		st.done_ = true;
	}
	return ret;
}

void base_operand::maybe_next_expr(state & st, t_tokens & toks, base_log * log) {
	if( check_kind<
		rk_operand,
		rk_operand_const,
		rk_operand_can_dot_get,
		rk_operand_can_dot_set,
		rk_separator
	>::check(st) ) {
		++st.expressions_count_;
		toks.append(new tokens::token_next_expr() );
		pa_del_flag<flag_was_colon>::post_action(st, toks, log);
	} else if( check_kind<
		rk_none,
		rk_prefix_operator
	>::check(st) ) {
		++st.expressions_count_;
	}
}

bool hive::fn_call_native::parse(state & st, t_tokens & toks, base_log * log) {
	if( *st.str_ != L'#' ) return false;
	bool ret = false;
	state st_tmp = st;
	st_tmp.str_ += 1;
	if( is_name::parse(st_tmp, toks, log) ) {
		ret = true;
		const Char * end = st_tmp.str_;
		bool is_bk = false;
		if( *end == L'>' ) {
			is_bk = true;
			end += 1;
		}
		toks.append(
			new tokens::token_add_fn_native(st.liner_.get_pos(st.str_), wtext(st.str_, st_tmp.str_), is_bk)
		);
		st.next_str(end);
		st.next_fn_ = rule_expr::parse;
	}
	return ret;
}

bool hive::fn_call_global::parse(state & st, t_tokens & toks, base_log * log) {
	if( *st.str_ != L'&' ) return false;
	bool ret = false;
	state st_tmp = st;
	st_tmp.str_ += 1;
	if( is_name::parse(st_tmp, toks, log) ) {
		ret = true;
		const Char * end = st_tmp.str_;
		bool is_bk = false;
		if( *end == L'>' ) {
			is_bk = true;
			end += 1;
		}
		toks.append(
			new tokens::token_add_fn_global(st.liner_.get_pos(st.str_), wtext(st.str_, st_tmp.str_), is_bk)
		);
		st.next_str(end);
		st.next_fn_ = rule_expr::parse;
	}
	return ret;
}

void lit_null::post_action(state & st, t_tokens & toks, base_log * log) {
	hive::operand::maybe_next_expr(st, toks, log);
	toks.append(new tokens::token_put_null(st.liner_.get_pos(st.prev_str_) ) );
}

void lit_bool::post_action(state & st, t_tokens & toks, base_log * log) {
	hive::operand::maybe_next_expr(st, toks, log);
	toks.append(new tokens::token_put_bool(st.liner_.get_pos(st.prev_str_), st.str_[-1] == L'1') );
}

bool lit_int::parse(state & st, t_tokens & toks, base_log * log) {
	const Char * begin = st.str_, * end;
	int base;
	if( !inner_parse(begin, end, base) )
	return false;

	wtext tx;
	if( *end == L'_' ) {
		tx = wtext(L"-", begin, end);
		end += 1;
	} else {
		tx = wtext(begin, end);
	}
	st.next_str(end);
	errno = 0;
	id num = std::wcstoll(tx.h_->cs_, nullptr, base);
	if( errno == ERANGE ) {
		errno = 0;
		log->add({
			L"warning: Literal of type $int beyond its limits.",
			st.inf_->path_,
			st.liner_.get_pos(st.prev_str_)
		});
	}
	hive::operand::maybe_next_expr(st, toks, log);
	toks.append(new tokens::token_put_int(st.liner_.get_pos(st.prev_str_), num) );
	return true;
}

bool lit_float::parse(state & st, t_tokens & toks, base_log * log) {
	if( !std::iswdigit(*st.str_) )
	return false;

	bool ret = false;
	const Char * begin = st.str_, * end;

	for( end = begin +1; std::iswdigit(*end); ++end );

	if( *end == L'.' && std::iswdigit(end[1]) ) {
		for( end += 2; std::iswdigit(*end); ++end );
		ret = true;
	}
	if( ret ) {
		if( *end == L'e' && (end[1] == L'+' || end[1] == L'-') && std::iswdigit(end[2]) ) {
			for( end += 3; std::iswdigit(*end); ++end );
		}
		wtext tx;
		if( *end == L'_' ) {
			tx = wtext(L"-", begin, end);
			end += 1;
		} else {
			tx = wtext(begin, end);
		}
		st.next_str(end);
		errno = 0;
		real num = WCS_TO_REAL(tx.h_->cs_, nullptr);
		if( errno == ERANGE ) {
			errno = 0;
			log->add({
				L"warning: Literal of type $float beyond its limits.",
				st.inf_->path_,
				st.liner_.get_pos(st.prev_str_)
			});
		}
		hive::operand::maybe_next_expr(st, toks, log);
		toks.append(new tokens::token_put_float(st.liner_.get_pos(st.prev_str_), num) );
	}
	return ret;
}

void lit_text_after_dot::post_action(state & st, t_tokens & toks, base_log * log) {
	toks.append(new tokens::token_put_text(st.liner_.get_pos(st.prev_str_), wtext(st.prev_str_, st.str_) ) );
}

bool lit_text_short::parse(state & st, t_tokens & toks, base_log * log) {
	if( *st.str_ != L'%' )
	return false;

	const Char * begin = st.str_ +1, * end;
	for( end = begin; test_char(*end); ++end );

	bool ret = end > begin;
	if( ret ) {
		st.next_str(end);
		hive::operand::maybe_next_expr(st, toks, log);
		toks.append(new tokens::token_put_text(st.liner_.get_pos(st.prev_str_), wtext(begin, end) ) );
	}
	return ret;
}

struct helper_text_literal {
	template <class S>
	static void test_newline(S & s, const Char * str, liner & ln) {
		if constexpr( std::is_same_v<S, ex::traits::s_counter> ) {
			if( (*str == L'\r' && str[1] != L'\n') || *str == L'\n' )
			ln.next_line(str + 1);
		}
	}

	// single-quoted
	template <class S>
	static bool parse_single(S & s, const Char * begin, const Char *& end, liner & ln, wtext & msg) {
		end = begin;
		bool ret = false;
		while( true ) {
			if( *end == 0 ) {
				msg = L"parse error: Unexpected EOF before closing single quote.";
				break;
			}
			if( *end == L'\'' ) {
				s.add(begin, end);
				end += 1;
				ret = true;
				break;
			}
			if( *end == L'\\' ) {
				if( end[1] == L'\'' || end[1] == L'\\' ) {
					s.add(begin, end);
					s << end[1];
					end += 2;
					begin = end;
				} else
				end += 1;
			} else {
				test_newline(s, end, ln);
				end += 1;
			}
		}
		return ret;
	}

	// double-quoted
	template <class S>
	static bool parse_double(S & s, const Char * begin, const Char *& end, liner & ln, wtext & msg) {
		end = begin;
		bool ret = false;
		while( true ) {
			if( *end == 0 ) {
				msg = L"parse error: Unexpected EOF before closing double quote.";
				break;
			}
			if( *end == L'"' ) {
				s.add(begin, end);
				end += 1;
				ret = true;
				break;
			}
			if( *end == L'\\' ) {
				Char ch = 0;
				const Char * the_end = end + 2;
				switch( end[1] ) {
				case L'\\':	ch = L'\\'; break;
				case L'"':	ch = L'"'; break;
				case L'n':	ch = L'\n'; break;
				case L'r':	ch = L'\r'; break;
				case L't':	ch = L'\t'; break;
				case L'v':	ch = L'\v'; break;
				case L'e':	ch = 27; break;
				case L'f':	ch = L'\f'; break;
				case L'u':
					if( end[2] != L'{' ) {
						msg = L"parse error: Double-quoted text literal contains illegal escape sequence. Char code format is: \\u{code}";
						end += 2;
						return false;
					}
					const Char * n_begin = end + 3, * n_end;
					int base;
					if( lit_int::inner_parse(n_begin, n_end, base) ) {
						if( *n_end != L'}' ) {
							msg = L"parse error: Double-quoted text literal contains illegal escape sequence. Char code format is: \\u{code}";
							end = n_end;
							return false;
						}
						id num = wcstoll(n_begin, nullptr, base);
						if( num < 1 || num > 65535 ) {
							msg = L"parse error: Double-quoted text literal escape sequence \\u{code} should contain code between 1 and 65535 inclusive.";
							end += 3;
							return false;
						}
						ch = static_cast<Char>(num);
						the_end = n_end +1;
					} else {
						msg = L"parse error: Double-quoted text literal contains illegal escape sequence. Char code format is: \\u{code}";
						end += 3;
						return false;
					}
					break;
				}
				if( ch ) {
					s.add(begin, end);
					s << ch;
					end = the_end;
					begin = end;
				} else
				end += 1;
			} else {
				test_newline(s, end, ln);
				end += 1;
			}
		}
		return ret;
	}
};

bool lit_text_single::parse(state & st, t_tokens & toks, base_log * log) {
	if( *st.str_ != L'\'' )
	return false;

	liner ln = st.liner_;
	const Char * end;
	wtext msg;
	ex::traits::s_counter s_cnt;
	bool ret = helper_text_literal::parse_single(s_cnt, st.str_ +1, end, ln, msg);
	if( ret ) {
		ex::traits::s_concater s_cat(s_cnt.cnt_);
		helper_text_literal::parse_single(s_cat, st.str_ +1, end, ln, msg);
		st.next_str(end);
		hive::operand::maybe_next_expr(st, toks, log);
		toks.append(new tokens::token_put_text(st.liner_.get_pos(st.prev_str_), s_cat.tx_) );
		st.liner_ = ln;
	} else if( msg ) {
		st.done_ = true;
		log->add({ msg, st.inf_->path_, ln.get_pos(end) });
	}
	return ret;
}

bool lit_text_double::parse(state & st, t_tokens & toks, base_log * log) {
	if( *st.str_ != L'"' )
	return false;

	liner ln = st.liner_;
	const Char * end;
	wtext msg;
	ex::traits::s_counter s_cnt;
	bool ret = helper_text_literal::parse_double(s_cnt, st.str_ +1, end, ln, msg);
	if( ret ) {
		ex::traits::s_concater s_cat(s_cnt.cnt_);
		helper_text_literal::parse_double(s_cat, st.str_ +1, end, ln, msg);
		st.next_str(end);
		hive::operand::maybe_next_expr(st, toks, log);
		toks.append(new tokens::token_put_text(st.liner_.get_pos(st.prev_str_), s_cat.tx_) );
		st.liner_ = ln;
	} else if( msg ) {
		st.done_ = true;
		log->add({ msg, st.inf_->path_, ln.get_pos(end) });
	}
	return ret;
}

void operand_type::post_action(state & st, t_tokens & toks, base_log * log) {
	hive::operand::maybe_next_expr(st, toks, log);
	toks.append(new tokens::token_put_type(st.liner_.get_pos(st.prev_str_), wtext(st.prev_str_ +1, st.str_) ) );
}

void variable_source::post_action(state & st, t_tokens & toks, base_log * log) {
	hive::operand::maybe_next_expr(st, toks, log);
	toks.append(new tokens::token_put_var(st.liner_.get_pos(st.prev_str_), wtext(st.prev_str_, st.str_) ) );
}

void variable_target::post_action(state & st, t_tokens & toks, base_log * log) {
	toks.append(new tokens::token_put_var_link(st.liner_.get_pos(st.prev_str_), wtext(st.prev_str_, st.str_) ) );
}

void hive::module_name::post_action(state & st, t_tokens & toks, base_log * log) {
	pa_add_flag<flag_module_has_name>::post_action(st, toks, log);
	toks.append( new tokens::token_set_module_name(ex::wtext(st.prev_str_ -1, st.str_)) );
}

void hive::fn_def::post_action(state & st, t_tokens & toks, base_log * log) {
	toks.append(new tokens::token_fn_begin(st.liner_.get_pos(st.prev_str_), wtext(st.prev_str_, st.str_) ) );
	st.next_fn_ = rule_fn_arg_1_or_type::parse;
}

bool hive::fn_def_type::parse(state & st, t_tokens & toks, base_log * log) {
	if( *st.str_ != L'$' ) return false;
	state st_tmp = st;
	st_tmp.next_str(st.str_ +1);
	if( is_name::parse(st_tmp, toks, log) ) {
		const Char * str = st_tmp.str_;
		wtext type_name(st.str_ +1, str);
		bool is_static = true;
		if( *str == L':' ) {
			++str;
			is_static = false;
		}
		st.next_str(str);
		toks.append(new tokens::token_fn_set_overload(st.liner_.get_pos(st.prev_str_), type_name, is_static) );
		return true;
	}
	return false;
}
void hive::fn_def_type::post_action(state & st, t_tokens & toks, base_log * log) {
	st.next_fn_ = rule_fn_arg<true>::parse;
}

} // ns
} // ns
