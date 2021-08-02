#pragma once
#include "rules.h"
#include <cwctype>

namespace ksi {
namespace rules {

struct state;

using hfn_check = bool (*)(state & st);
using hfn_parse = bool (*)(state & st, t_tokens & toks, base_log * log);
using hfn_get_parse = hfn_parse (*)();
using hfn_post_action = void (*)(state & st, t_tokens & toks, base_log * log);
using hfn_message = wtext (*)(state & st, Char ch);
using hfn_name = wtext (*)(state & st);

struct liner {
	const Char * line_start_;
	id line_no_ = 1;

	void next_line(const Char * str) {
		line_start_ = str;
		line_no_ += 1;
	}

	also::t_pos get_pos(const Char * str) {
		return {line_no_, str - line_start_ +1};
	}
};

enum flag : uid {
	flag_has_module_info	= 1 << 0,
	flag_module_has_name	= 1 << 1,
	flag_was_colon			= 1 << 2,
	flag_condition_else		= 1 << 3,
	flag_second_part		= 1 << 4, // after '\'
	flag_was_each_order_sign= 1 << 5,
	flag_was_prefix_operator= 1 << 6
};

enum n_rule_kind : uid {
	rk_none,
	rk_space,
	rk_keep,
	rk_operand,
	rk_operand_can_dot_get,
	rk_operand_can_dot_set,
	rk_operator,
	rk_prefix_operator,
	rk_fn_call,
	rk_separator
};

template <n_rule_kind Kind>
struct with_kind {
	constexpr static n_rule_kind kind = Kind;
};

enum n_expr_nest {
	nest_plain,
	nest_function,
	nest_parentheses,
	nest_array,
	nest_map,
	nest_bracket,
	nest_condition,
	nest_while_condition,
	nest_loop_body
};

template <uid Flag>
struct flag_traits {
	enum n : uid { neg_flag = ~Flag };
};

struct state {
	info * inf_;
	const Char * str_;
	const Char * prev_str_;
	liner liner_;
	id loop_depth_;
	hfn_parse next_fn_;
	n_expr_nest nest_ = nest_plain;
	uid flags_ = 0;
	n_rule_kind prev_rule_ = rk_none;
	id expressions_count_ = 0;
	bool was_od_ = false;
	bool done_ = false;

	void next_str(const Char * str) {
		prev_str_ = str_;
		str_ = str;
	}
};

// check

struct check_none {
	static bool check(state & st) {
		return true;
	}
};

template <uid Flag>
struct check_flag_is_absent {
	static bool check(state & st) {
		return !(st.flags_ & Flag);
	}
};

template <bool Was_od>
struct check_was_od {
	static bool check(state & st) {
		return st.was_od_ == Was_od;
	}
};

//

template <n_rule_kind ... Items>
struct check_kind;

template <n_rule_kind First, n_rule_kind ... Rest>
struct check_kind<First, Rest...> {
	static bool check(state & st) {
		return st.prev_rule_ == First || check_kind<Rest...>::check(st);
	}
};

template <n_rule_kind Last>
struct check_kind<Last> {
	static bool check(state & st) {
		return st.prev_rule_ == Last;
	}
};

//

template <n_rule_kind ... Items>
struct check_kind_not;

template <n_rule_kind First, n_rule_kind ... Rest>
struct check_kind_not<First, Rest...> {
	static bool check(state & st) {
		return st.prev_rule_ != First && check_kind_not<Rest...>::check(st);
	}
};

template <n_rule_kind Last>
struct check_kind_not<Last> {
	static bool check(state & st) {
		return st.prev_rule_ != Last;
	}
};

//

template <class ... Items>
struct check_and;

template <class First, class ... Rest>
struct check_and<First, Rest...> {
	static bool check(state & st) {
		return First::check(st) && check_and<Rest...>::check(st);
	}
};

template <class Last>
struct check_and<Last> {
	static bool check(state & st) {
		return Last::check(st);
	}
};


// post action

template <uid Flag>
struct pa_add_flag {
	static void post_action(state & st, t_tokens & toks, base_log * log) {
		st.flags_ |= Flag;
	}
};

template <uid Flag>
struct pa_del_flag {
	static void post_action(state & st, t_tokens & toks, base_log * log) {
		st.flags_ &= flag_traits<Flag>::neg_flag;
	}
};

struct pa_none {
	static void post_action(state & st, t_tokens & toks, base_log * log) {}
};

// pa_seq

template <class ... Items>
struct pa_seq;

template <class First, class ... Rest>
struct pa_seq<First, Rest...> {
	static void post_action(state & st, t_tokens & toks, base_log * log) {
		First::post_action(st, toks, log);
		pa_seq<Rest...>::post_action(st, toks, log);
	}
};

template <class Last>
struct pa_seq<Last> {
	static void post_action(state & st, t_tokens & toks, base_log * log) {
		Last::post_action(st, toks, log);
	}
};

//

template <hfn_parse Fn>
struct pa_next_fn {
	static void post_action(state & st, t_tokens & toks, base_log * log) {
		st.next_fn_ = Fn;
	}
};

//

struct pa_done_good {
	static void post_action(state & st, t_tokens & toks, base_log * log) {
		st.inf_->good_ = true;
		st.done_ = true;
	}
};

//

struct od :
public with_kind<rk_space>,
public check_was_od<false>,
public pa_none {
	static wtext name(state & st) { return L"t_whitespace"; }
	static bool is_comment_begin(const Char *& str, id & depth) {
		bool ret = false;
		if( *str == L'/' && str[1] == L'*' ) {
			depth += 1;
			str += 2;
			ret = true;
		}
		return ret;
	}
	static bool is_comment_end(const Char *& str, id & depth) {
		bool ret = false;
		if( *str == L'*' && str[1] == L'/' ) {
			depth -= 1;
			str += 2;
			ret = true;
		}
		return ret;
	}
	static void check_newline(const Char * str, state & st) {
		if( (*str == L'\r' && str[1] != L'\n') || *str == L'\n' )
		st.liner_.next_line(str + 1);
	}
	static bool parse(state & st, t_tokens & toks, base_log * log) {
		bool ret = false;
		const Char * str = st.str_;
		id depth = 0;
		while(
			std::iswspace(*str) ||
			(*str == L'-' && str[1] == L'-') ||
			is_comment_begin(str, depth)
		) {
			if( depth ) {
				while( *str != 0 && depth > 0 ) {
					if( is_comment_end(str, depth) || is_comment_begin(str, depth) )
					continue;
					check_newline(str, st);
					str += 1;
				}
			} else if( *str == L'-' ) {
				str += 2;
				while( *str != L'\r' && *str != L'\n' && *str != 0 )
				str += 1;
			} else {
				check_newline(str, st);
				str += 1;
			}
		}
		if( str > st.str_ ) {
			ret = true;
			st.next_str(str);
		}
		return ret;
	}
};

struct end_file :
public with_kind<rk_none>,
public check_none,
public pa_done_good {
	static wtext name(state & st) { return L"t_eof"; }
	static bool parse(state & st, t_tokens & toks, base_log * log) {
		bool ret = *st.str_ == 0;
		return ret;
	}
};

template<Char Ch>
struct is_char {
	static bool parse(state & st, t_tokens & toks, base_log * log) {
		bool ret = *st.str_ == Ch;
		if( ret )
		st.next_str(st.str_ +1);
		return ret;
	}
};

struct is_name {
	static wtext name(state & st) { return L"t_identifier"; }
	static bool parse(state & st, t_tokens & toks, base_log * log) {
		bool ret = false;
		const Char * str = st.str_;
		if( *str == L'_' || std::iswalpha(*str) ) {
			str += 1;
			while( *str == L'_' || std::iswalnum(*str) )
			str += 1;
		}
		if( str > st.str_ ) {
			ret = true;
			st.next_str(str);
		}
		return ret;
	}
};

// base_rule

struct base_rule {
	static void go(state & st, t_tokens & toks, base_log * log) {
		while( !st.done_ )
		st.next_fn_(st, toks, log);
	}
	static inline wtext get_message(Char ch) {
		return ch == 0 ? L"EOF." : (
			std::iswspace(ch) ? L"whitespace." : (
				ex::implode({L"char: ", ch})
			)
		);
	}
	static wtext get_message(const ex::wtext_array & items, Char ch) {
		switch( items.count_ ) {
		case 0:
			return ex::implode({ L"parse error: Expected nothing, got ", get_message(ch) });
			break;
		case 1:
			return ex::implode({ L"parse error: Expected ", items.items_[0], L", got ", get_message(ch) });
			break;
		default:
			return ex::implode({
				L"parse error: Expected one of the following (",
				ex::implode(items, L", "),
				L"), got ",
				get_message(ch)
			});
		}
	}
};

// rule_alt

template <bool Change_rule_kind, bool None_match_done, class ... Items>
struct rule_alt;

template <bool Change_rule_kind, bool None_match_done, class First, class ... Rest>
struct rule_alt<Change_rule_kind, None_match_done, First, Rest...> {
	static wtext get_message(state & st, Char ch) {
		ex::wtext_array items(sizeof...(Rest) +1, 1);
		add_items(st, items);
		return base_rule::get_message(items, ch);
	}
	static void add_items(state & st, ex::wtext_array & items) {
		if( First::check(st) )
		items.append( First::name(st) );
		rule_alt<Change_rule_kind, None_match_done, Rest...>::add_items(st, items);
	}

	static bool inner_parse(state & st, t_tokens & toks, base_log * log, hfn_message fn) {
		if( First::check(st) ) {
			bool ret = First::parse(st, toks, log);
			if( ret ) {
				First::post_action(st, toks, log);
				if constexpr( Change_rule_kind ) {
					if constexpr( First::kind == rk_space )
					st.was_od_ = true;
					else if constexpr( First::kind != rk_keep ) {
						st.prev_rule_ = First::kind;
						st.was_od_ = false;
					}
				}
				return true;
			}
		}
		return st.done_ ? false : rule_alt<Change_rule_kind, None_match_done, Rest...>::inner_parse(st, toks, log, fn);
	}
	static bool parse(state & st, t_tokens & toks, base_log * log) {
		return inner_parse(st, toks, log, get_message);
	}
};

template <bool Change_rule_kind, bool None_match_done, class Last>
struct rule_alt<Change_rule_kind, None_match_done, Last> {
	static wtext get_message(state & st, Char ch) {
		ex::wtext_array items(1, 1);
		add_items(st, items);
		return base_rule::get_message(items, ch);
	}
	static void add_items(state & st, ex::wtext_array & items) {
		if( Last::check(st) )
		items.append( Last::name(st) );
	}

	static bool inner_parse(state & st, t_tokens & toks, base_log * log, hfn_message fn) {
		bool ret = false;
		if( Last::check(st) ) {
			ret = Last::parse(st, toks, log);
			if( ret ) {
				Last::post_action(st, toks, log);
				if constexpr( Change_rule_kind ) {
					if constexpr( Last::kind == rk_space )
					st.was_od_ = true;
					else if constexpr( Last::kind != rk_keep ) {
						st.prev_rule_ = Last::kind;
						st.was_od_ = false;
					}
				}
			}
		}
		if constexpr( None_match_done ) {
			if( !ret && !st.done_ ) {
				st.done_ = true;
				log->add( {fn(st, *st.str_), st.inf_->path_, st.liner_.get_pos(st.str_)} );
			}
		}
		return ret;
	}
	static bool parse(state & st, t_tokens & toks, base_log * log) {
		return inner_parse(st, toks, log, get_message);
	}
};

} // ns
} // ns
