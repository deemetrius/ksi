#pragma once
#include "rules.inc.h"
#include "tokens.inc.h"
#include <errno.h>

namespace ksi {
namespace rules {

struct base_operand {
	static void maybe_next_expr(state & st, t_tokens & toks, base_log * log);
};

//

struct lit_null :
public is_char<L'#'>,
public check_none {
	static wtext name(state & st) { return L"t_literal_null"; }

	static void post_action(state & st, t_tokens & toks, base_log * log);
};

struct lit_bool :
public check_none {
	static wtext name(state & st) { return L"t_literal_bool"; }

	static bool parse(state & st, t_tokens & toks, base_log * log) {
		bool ret = false;
		if( *st.str_ == L'#' && (st.str_[1] == L'0' || st.str_[1] == L'1') ) {
			ret = true;
			st.next_str(st.str_ +2);
		}
		return ret;
	}

	static void post_action(state & st, t_tokens & toks, base_log * log);
};

struct lit_int :
public check_none,
public pa_none {
	static wtext name(state & st) { return L"t_literal_int"; }

	static void maybe_fix_range(const Char * the_begin, const Char *& begin, const Char *& end, int & var_base, int to_base) {
		if( end > begin )
		var_base = to_base;
		else {
			begin = the_begin;
			end = begin +1;
		}
	}
	static bool inner_parse(const Char *& begin, const Char *& end, int & base) {
		if( !std::iswdigit(*begin) )
		return false;

		const Char * the_begin = begin;
		base = 10;
		if( *the_begin == L'0' ) {
			switch( the_begin[1] ) {
			case L'b':
				begin += 2;
				for( end = begin; *end == L'0' || *end == L'1'; ++end );
				maybe_fix_range(the_begin, begin, end, base, 2);
				break;
			case L'o':
				begin += 2;
				for( end = begin; *end >= L'0' && *end <= L'7'; ++end );
				maybe_fix_range(the_begin, begin, end, base, 8);
				break;
			case L'h':
				begin += 2;
				for( end = begin; std::iswxdigit(*end); ++end );
				maybe_fix_range(the_begin, begin, end, base, 16);
				break;
			default:
				for( end = begin +1; std::iswdigit(*end); ++end );
			}
		} else {
			for( end = begin +1; std::iswdigit(*end); ++end );
		}
		return true;
	}
	static bool parse(state & st, t_tokens & toks, base_log * log);
};

struct lit_float :
public check_none,
public pa_none {
	static wtext name(state & st) { return L"t_literal_float"; }

	static bool parse(state & st, t_tokens & toks, base_log * log);
};

struct lit_text_after_dot :
public check_none,
public is_name {
	static wtext name(state & st) { return L"t_literal_text_key"; }

	static void post_action(state & st, t_tokens & toks, base_log * log);
};

// %0 %_1 %x %text %_hello_
struct lit_text_short :
public check_none,
public pa_none {
	static wtext name(state & st) { return L"t_literal_text_short"; }

	static bool test_char(Char ch) {
		return ch == L'_' || std::iswalnum(ch);
	}

	static bool parse(state & st, t_tokens & toks, base_log * log);
};

// single-quoted
struct lit_text_single :
public check_none,
public pa_none {
	static wtext name(state & st) { return L"t_literal_text_single_quoted"; }

	static bool parse(state & st, t_tokens & toks, base_log * log);
};

// double-quoted
struct lit_text_double :
public check_none,
public pa_none {
	static wtext name(state & st) { return L"t_literal_text_double_quoted"; }

	static bool parse(state & st, t_tokens & toks, base_log * log);
};

//

struct operand_type :
public check_none {
	static wtext name(state & st) { return L"t_type"; }

	static bool parse(state & st, t_tokens & toks, base_log * log) {
		bool ret = false;
		if( *st.str_ == L'$' ) {
			const Char * str = st.str_ +1;
			state st_tmp = st;
			st_tmp.next_str(str);
			if( is_name::parse(st_tmp, toks, log) ) {
				st.next_str(st_tmp.str_);
				ret = true;
			}
		}
		return ret;
	}

	static void post_action(state & st, t_tokens & toks, base_log * log);
};

struct operand_type_const :
public check_none,
public pa_none {
	static wtext name(state & st) { return L"t_type_const"; }

	static bool parse(state & st, t_tokens & toks, base_log * log) {
		state tmp_st = st;
		if( !operand_type::parse(tmp_st, toks, log) ) return false;
		also::t_pos pos = tmp_st.liner_.get_pos(tmp_st.prev_str_);
		wtext type_name(tmp_st.prev_str_ +1, tmp_st.str_);
		// maybe spaces
		od::parse(tmp_st, toks, log);
		// dot
		if( *tmp_st.str_ != L'.' ) return false;
		tmp_st.next_str(tmp_st.str_ +1);
		// maybe spaces
		od::parse(tmp_st, toks, log);
		// const name
		if( !is_name::parse(tmp_st, toks, log) ) return false;
		wtext const_name(tmp_st.prev_str_, tmp_st.str_);
		// #
		if( *tmp_st.str_ != L'#' ) return false;
		// done
		st.next_str(tmp_st.str_ +1);
		base_operand::maybe_next_expr(st, toks, log);
		toks.append( new tokens::token_put_type_const(pos, type_name, const_name) );
		return true;
	}
};

//

struct variable_source :
public with_kind<rk_operand_can_dot_get>,
public check_none,
public is_name {
	static wtext name(state & st) { return L"t_variable"; }

	static void post_action(state & st, t_tokens & toks, base_log * log);
};

struct variable_target :
public with_kind<rk_operand_can_dot_set>,
public check_none,
public is_name {
	static wtext name(state & st) { return L"t_variable"; }

	static void post_action(state & st, t_tokens & toks, base_log * log);
};

//

template <class T>
struct is_keyword {
	using self_base_is_keyword = is_keyword;

	static bool text_compare(const wtext & tx, const Char * str) {
		return !std::wcscmp(tx.h_->cs_, str);
	}

	static bool parse(state & st, t_tokens & toks, base_log * log) {
		bool ret = false;
		if( *st.str_ == L'`' ) {
			const Char * str = st.str_ +1;
			state st_tmp = st;
			st_tmp.next_str(str);
			if( is_name::parse(st_tmp, toks, log) ) {
				wtext tx(st.str_, st_tmp.str_);
				if( T::check_text(tx, st, toks, log) ) {
					st.next_str(st_tmp.str_);
					ret = true;
				}
			}
		}
		return ret;
	}
};

//

struct t_operator_std :
public check_none,
public pa_none {
	static wtext name(state & st) { return L"t_operator_std"; }

	static bool parse(state & st, t_tokens & toks, base_log * log);
};

struct t_prefix_operator_std :
public check_none,
public pa_none {
	static wtext name(state & st) { return L"t_prefix_operator_std"; }

	static bool parse(state & st, t_tokens & toks, base_log * log);
};

struct t_operator_named :
public is_keyword<t_operator_named>,
public check_none,
public pa_none {
	static wtext name(state & st) { return L"t_operator_named"; }

	static bool check_text(const wtext & tx, state & st, t_tokens & toks, base_log * log);
};

struct t_prefix_operator_named :
public is_keyword<t_prefix_operator_named>,
public check_none,
public pa_none {
	static wtext name(state & st) { return L"t_prefix_operator_named"; }

	static bool check_text(const wtext & tx, state & st, t_tokens & toks, base_log * log);
};

struct for_dot_get {
	static void add_token(state & st, t_tokens & toks);
	static void add_bracket_begin(state & st, t_tokens & toks);
	static void add_bracket_end(state & st, t_tokens & toks);
};

struct for_dot_set {
	static void add_token(state & st, t_tokens & toks);
	static void add_bracket_begin(state & st, t_tokens & toks);
};

//

struct end_plain :
public end_file {
	static void post_action(state & st, t_tokens & toks, base_log * log);
};

struct end_fn :
public with_kind<rk_none>,
public check_none,
public is_char<L';'>,
public pa_done_good {
	static wtext name(state & st) { return L"t_end_fn"; }
};

struct end_scope :
public with_kind<rk_operand>,
public is_char<L')'>,
public pa_done_good {
	static bool check(state & st) {
		return check_flag_is_absent<flag_was_prefix_operator>::check(st) || st.expressions_count_ > 1;
	}

	static wtext name(state & st) { return L"t_end_scope"; }
};

struct end_array :
public with_kind<rk_operand>,
public check_none,
public is_char<L']'>,
public pa_done_good {
	static wtext name(state & st) { return L"t_end_array"; }
};

struct end_map :
public with_kind<rk_operand>,
public check_none,
public is_char<L'}'>,
public pa_done_good {
	static wtext name(state & st) { return L"t_end_map"; }
};

struct end_bracket :
public with_kind<rk_operand>,
public check_none,
public is_char<L']'>,
public pa_done_good {
	static wtext name(state & st) { return L"t_end_bracket"; }
};

struct end_condition :
public with_kind<rk_operand>,
public check_none,
public pa_done_good {
	static wtext name(state & st) {
		if( check_flag_is_absent<flag_condition_else>::check(st) ) {
			return L"t_else_or_end_condition";
		} else if( check_kind_not<rk_none, rk_separator>::check(st) ) {
			return L"t_then_or_end_condition";
		}
		return L"t_end_condition";
	}

	static bool parse(state & st, t_tokens & toks, base_log * log) {
		bool ret = false;
		if( *st.str_ == L';' ) {
			ret = true;
		} else if( check_flag_is_absent<flag_condition_else>::check(st) ) {
			if( *st.str_ == L'|' )
			ret = true;
		} else {
			if( *st.str_ == L'?' && check_kind_not<rk_none, rk_separator>::check(st) )
			ret = true;
		}
		if( ret )
		st.next_str(st.str_ +1);
		return ret;
	}
};

struct end_while_condition :
public with_kind<rk_operand>,
public check_none,
public pa_done_good {
	static wtext name(state & st) {
		if( check_flag_is_absent<flag_second_part>::check(st) ) {
			return L"t_begin_additional_block_or_begin_loop_body";
		}
		return L"t_begin_loop_body";
	}

	static bool parse(state & st, t_tokens & toks, base_log * log) {
		bool ret = false;
		if( *st.str_ == L'\\' ) {
			ret = check_flag_is_absent<flag_second_part>::check(st);
		} else if( *st.str_ == L'~' ) {
			ret = true;
		}
		if( ret )
		st.next_str(st.str_ +1);
		return ret;
	}
};

struct end_loop_body :
public with_kind<rk_operand>,
public check_none,
public pa_done_good {
	static wtext name(state & st) {
		if( check_flag_is_absent<flag_second_part>::check(st) ) {
			return L"t_loop_body_separator_or_end_loop_body";
		}
		return L"t_end_loop_body";
	}

	static bool parse(state & st, t_tokens & toks, base_log * log) {
		bool ret = false;
		if( *st.str_ == L'\\' ) {
			ret = check_flag_is_absent<flag_second_part>::check(st);
		} else if( *st.str_ == L';' ) {
			ret = true;
		}
		if( ret )
		st.next_str(st.str_ +1);
		return ret;
	}
};

struct end_expr :
public with_kind<rk_keep> {
	static bool check(state & st) {
		static constexpr hfn_check items[] = {
			end_plain::check, end_fn::check,
			end_scope::check, end_array::check, end_map::check,
			end_bracket::check, end_condition::check,
			end_while_condition::check, end_loop_body::check
		};
		return check_kind_not<rk_operator, rk_fn_call>::check(st) && items[st.nest_](st);
	}

	static wtext name(state & st) {
		static constexpr hfn_name items[] = {
			end_plain::name, end_fn::name,
			end_scope::name, end_array::name, end_map::name,
			end_bracket::name, end_condition::name,
			end_while_condition::name, end_loop_body::name
		};
		return items[st.nest_](st);
	}

	static bool parse(state & st, t_tokens & toks, base_log * log) {
		static constexpr hfn_parse items[] = {
			end_plain::parse, end_fn::parse,
			end_scope::parse, end_array::parse, end_map::parse,
			end_bracket::parse, end_condition::parse,
			end_while_condition::parse, end_loop_body::parse
		};
		return items[st.nest_](st, toks, log);
	}

	static void post_action(state & st, t_tokens & toks, base_log * log) {
		static constexpr hfn_post_action items[] = {
			end_plain::post_action, end_fn::post_action,
			end_scope::post_action, end_array::post_action, end_map::post_action,
			end_bracket::post_action, end_condition::post_action,
			end_while_condition::post_action, end_loop_body::post_action
		};
		static constexpr n_rule_kind kinds[] = {
			end_plain::kind, end_fn::kind,
			end_scope::kind, end_array::kind, end_map::kind,
			end_bracket::kind, end_condition::kind,
			end_while_condition::kind, end_loop_body::kind
		};
		items[st.nest_](st, toks, log);
		st.prev_rule_ = kinds[st.nest_];
		st.was_od_ = false;
	}
};

struct hive {

	// module_info

	struct module_info_begin :
	public with_kind<rk_none>,
	public is_char<L'@'>,
	public check_flag_is_absent<flag_has_module_info> {
		static wtext name(state & st) { return L"t_module_info"; }

		static void post_action(state & st, t_tokens & toks, base_log * log) {
			pa_seq< pa_add_flag<flag_has_module_info> >::post_action(st, toks, log);
			st.next_fn_ = rule_module_info::parse;
		}
	};

	struct module_name :
	public with_kind<rk_none>,
	public is_name,
	public check_and< check_was_od<false>, check_flag_is_absent<flag_module_has_name> > {
		static void post_action(state & st, t_tokens & toks, base_log * log);
	};

	struct end_module_info :
	public with_kind<rk_none>,
	public is_char<L';'>,
	public check_none {
		static wtext name(state & st) { return L"t_module_info_end"; }

		static void post_action(state & st, t_tokens & toks, base_log * log) {
			st.next_fn_ = rule_file::parse;
		}
	};

	// fn

	struct fn_def :
	public with_kind<rk_none>,
	public check_none {
		static wtext name(state & st) { return L"t_function_definition"; }

		static bool parse(state & st, t_tokens & toks, base_log * log) {
			if( *st.str_ != L'&' ) return false;
			state st_tmp = st;
			st_tmp.next_str(st.str_ +1);
			if( is_name::parse(st_tmp, toks, log) ) {
				st.next_str(st_tmp.str_);
				return true;
			}
			return false;
		}

		static void post_action(state & st, t_tokens & toks, base_log * log);
	};

	struct fn_def_type :
	public with_kind<rk_none>,
	public check_none {
		static wtext name(state & st) { return L"t_function_definition_type"; }

		static bool parse(state & st, t_tokens & toks, base_log * log);

		static void post_action(state & st, t_tokens & toks, base_log * log);
	};

	template <bool Is_first>
	struct fn_def_arg :
	public with_kind<rk_none>,
	public check_none,
	public is_name {
		static wtext name(state & st) {
			if constexpr( Is_first )
			return L"t_function_definition_arg_1";
			else
			return L"t_function_definition_arg_2";
		}

		static void post_action(state & st, t_tokens & toks, base_log * log) {
			toks.append(new tokens::token_fn_add_arg<Is_first>(
				st.liner_.get_pos(st.prev_str_),
				wtext(st.prev_str_, st.str_)
			) );
			if constexpr( Is_first )
			st.next_fn_ = rule_fn_arg<false>::parse;
			else
			st.next_fn_ = rule_fn_body::parse;
		}
	};

	struct fn_def_body :
	public with_kind<rk_none>,
	public check_none {
		static wtext name(state & st) { return L"t_function_definition_body"; }

		static bool parse(state & st, t_tokens & toks, base_log * log) {
			if( *st.str_ != L'~' ) return false;
			const Char * str = st.str_ +1;
			info tmp_inf = { st.inf_->path_ };
			state tmp_st = {
				&tmp_inf,
				str,
				str,
				st.liner_,
				0,
				rule_expr::parse,
				nest_function
			};
			base_rule::go(tmp_st, toks, log);
			if( tmp_inf.good_ ) {
				toks.append( new tokens::token_fn_end() );
				st.next_str(tmp_st.str_);
				st.liner_ = tmp_st.liner_;
				return true;
			}
			st.done_ = true;
			return false;
		}

		static void post_action(state & st, t_tokens & toks, base_log * log) {
			st.next_fn_ = rule_file::parse;
		}
	};

	// expr

	struct keyword_plain :
	public with_kind<rk_none>,
	public is_keyword<keyword_plain>,
	public check_none {
		static wtext name(state & st) { return L"t_keyword_plain"; }

		static bool check_text(const wtext & tx, state & st, t_tokens & toks, base_log * log) {
			return text_compare(tx, L"`plain");
		}

		static void post_action(state & st, t_tokens & toks, base_log * log);
	};

	struct operand_scope :
	public check_none,
	public pa_none {
		static wtext name(state & st) { return L"t_parentheses"; }

		static bool parse(state & st, t_tokens & toks, base_log * log);
	};

	struct operand_array :
	public check_none,
	public pa_none {
		static wtext name(state & st) { return L"t_array"; }

		static bool parse(state & st, t_tokens & toks, base_log * log);
	};

	struct operand_map :
	public check_none,
	public pa_none {
		static wtext name(state & st) { return L"t_map"; }

		static bool parse(state & st, t_tokens & toks, base_log * log);
	};

	struct operand_normal :
	public with_kind<rk_operand>,
	public check_none,
	public rule_alt<false, false,
		lit_bool, lit_null, lit_float, lit_int, lit_text_short, lit_text_single, lit_text_double,
		operand_type_const, operand_type,
		operand_scope, operand_array, operand_map
	>,
	public pa_none {
		static wtext name(state & st) { return L"t_operand_normal"; }
	};

	struct operand_loop_while :
	public with_kind<rk_operand>,
	public is_keyword<operand_loop_while>,
	public check_none,
	public pa_none {
		static wtext name(state & st) { return L"t_loop_while"; }

		static bool check_text(const wtext & tx, state & st, t_tokens & toks, base_log * log) {
			return text_compare(tx, L"`while");
		}

		static bool parse(state & st, t_tokens & toks, base_log * log);
	};

	// next, break

	struct base_kw_loop_special :
	public with_kind<rk_operand>,
	public pa_none {
		static bool check(state & st) {
			return st.loop_depth_;
		}
	};

	template <class T>
	struct is_kw_loop_special :
	public base_kw_loop_special {
		using self_base_is_keyword = is_keyword<T>;

		static bool check_text(const wtext & tx, state & st, t_tokens & toks, base_log * log) {
			return self_base_is_keyword::text_compare(tx, T::str_keyword);
		}

		static bool parse(state & st, t_tokens & toks, base_log * log) {
			const Char * str;
			//
			{
				state tmp_st = st;
				if( !self_base_is_keyword::parse(tmp_st, toks, log) ) return false;
				str = tmp_st.str_;
			}
			id depth = 1;
			if( *str == L'=' ) {
				++str;
				const Char * end;
				for( end = str; std::iswdigit(*end); ++end );
				if( str == end ) {
					log->add({ ex::implode({
						L"parse error: Expected depth after ", T::str_keyword, L"="}),
						st.inf_->path_,
						st.liner_.get_pos(str)
					});
					st.done_ = true;
					return false;
				}
				wtext tx_depth(str, end);
				errno = 0;
				depth = std::wcstoll(tx_depth.h_->cs_, nullptr, 10);
				if( errno == ERANGE || depth < 1 || depth > st.loop_depth_ ) {
					errno = 0;
					log->add({
						ex::implode({
							L"parse error: Keyword ", T::str_keyword, L"=depth (depth should be in range from 1 to ",
							ex::to_wtext(st.loop_depth_), L")"
						}),
						st.inf_->path_,
						st.liner_.get_pos(str)
					});
					st.done_ = true;
					return false;
				}
				str = end;
			}
			st.next_str(str);
			base_operand::maybe_next_expr(st, toks, log);
			toks.append(new typename T::t_token(T::get_instr_type(), st.liner_.get_pos(st.prev_str_), depth) );
			return true;
		}
	};

	struct operand_kw_next :
	public is_kw_loop_special<operand_kw_next> {
		using t_token = tokens::token_kw_next;
		static wtext name(state & st) { return L"t_kw_next"; }
		static constexpr const Char str_keyword[] = L"`next";
		static const mod::instr_type * get_instr_type();
	};

	struct operand_kw_break :
	public is_kw_loop_special<operand_kw_break> {
		using t_token = tokens::token_kw_break;
		static wtext name(state & st) { return L"t_kw_break"; }
		static constexpr const Char str_keyword[] = L"`break";
		static const mod::instr_type * get_instr_type();
	};

	//

	struct operand_kw_return :
	public with_kind<rk_operand>,
	public check_none,
	public is_keyword<operand_kw_return> {
		static wtext name(state & st) { return L"t_kw_return"; }

		static bool check_text(const wtext & tx, state & st, t_tokens & toks, base_log * log) {
			return text_compare(tx, L"`return");
		}

		static void post_action(state & st, t_tokens & toks, base_log * log) {
			base_operand::maybe_next_expr(st, toks, log);
			toks.append(new tokens::token_kw_return(st.liner_.get_pos(st.prev_str_) ) );
		}
	};

	//

	struct operand :
	public with_kind<rk_keep>,
	public check_none,
	public base_operand,
	public rule_alt<true, false,
		operand_normal, variable_source, operand_loop_while, operand_kw_next, operand_kw_break, operand_kw_return
	> {
		static wtext name(state & st) { return L"t_operand"; }

		static void post_action(state & st, t_tokens & toks, base_log * log) {
			st.next_fn_ = rule_expr_after_operand::parse;
		}
	};

	template <bool In_assign>
	struct operand_target :
	public with_kind<rk_keep>,
	public check_none,
	public rule_alt<true, false, variable_target> {
		static wtext name(state & st) { return L"t_assign_target"; }

		static void post_action(state & st, t_tokens & toks, base_log * log) {
			if constexpr( In_assign )
			st.next_fn_ = rule_expr_assign_after_operand::parse;
			else
			st.next_fn_ = rule_expr_after_operand::parse;
		}
	};

	struct operation_assign :
	public with_kind<rk_operator>,
	public check_none,
	public pa_none {
		static wtext name(state & st) { return L"t_assign_expression"; }

		static bool parse(state & st, t_tokens & toks, base_log * log) {
			t_tokens tmp_toks;
			log_last_only tmp_log;
			info tmp_inf = { st.inf_->path_ };
			state tmp_st = {
				&tmp_inf,
				st.str_,
				st.str_,
				st.liner_,
				st.loop_depth_,
				rule_expr_assign::parse
			};
			base_rule::go(tmp_st, tmp_toks, &tmp_log);
			bool ret = false;
			if( tmp_inf.good_ ) {
				hive::operand::maybe_next_expr(st, toks, log);
				for( tokens::base_token *& it : tmp_toks ) {
					toks.append(it);
					it = nullptr;
				}
				st.next_str(tmp_st.str_);
				st.liner_ = tmp_st.liner_;
				st.next_fn_ = rule_expr::parse;
				ret = true;
			}
			return ret;
		}
	};

	struct operation_condition :
	public with_kind<rk_operand>,
	public pa_none {
		static wtext name(state & st) { return L"t_then"; }

		static bool check(state & st) {
			return st.nest_ != nest_condition;
		}

		static bool parse(state & st, t_tokens & toks, base_log * log);
	};

	struct operation_loop_each :
	public with_kind<rk_operand>,
	public check_none,
	public is_keyword<operation_loop_each>,
	public pa_none {
		static wtext name(state & st) { return L"t_loop_each"; }

		static bool check_text(const wtext & tx, state & st, t_tokens & toks, base_log * log) {
			return text_compare(tx, L"`each");
		}

		static bool parse(state & st, t_tokens & toks, base_log * log);
	};

	struct operation_loop_each_key_sign :
	public with_kind<rk_none>,
	public check_flag_is_absent<flag_was_each_order_sign>,
	public pa_add_flag<flag_was_each_order_sign> {
		static wtext name(state & st) { return L"t_loop_each_key_sign"; }

		static bool parse(state & st, t_tokens & toks, base_log * log) {
			bool ret = false;
			id order;
			if( *st.str_ == L'+' ) {
				ret = true;
				order = var::order_key_asc;
			} else if( *st.str_ == L'-' ) {
				ret = true;
				order = var::order_key_desc;
			}
			if( ret ) {
				st.next_str(st.str_ +1);
				toks.append(new tokens::token_loop_each_order(order) );
			}
			return ret;
		}
	};

	struct operation_loop_each_val_sign :
	public with_kind<rk_none>,
	public check_flag_is_absent<flag_was_each_order_sign>,
	public pa_add_flag<flag_was_each_order_sign> {
		static wtext name(state & st) { return L"t_loop_each_value_sign"; }

		static bool parse(state & st, t_tokens & toks, base_log * log) {
			bool ret = false;
			if( *st.str_ == L'-' ) {
				ret = true;
				st.next_str(st.str_ +1);
				toks.append(new tokens::token_loop_each_order(var::order_desc) );
			}
			return ret;
		}
	};

	struct operation_loop_each_key :
	public with_kind<rk_none>,
	public check_none,
	public is_name {
		static wtext name(state & st) { return L"t_loop_each_key_variable"; }

		static void post_action(state & st, t_tokens & toks, base_log * log) {
			toks.append(new tokens::token_loop_each_key(st.liner_.get_pos(st.prev_str_), wtext(st.prev_str_, st.str_) ) );
			st.next_fn_ = rule_loop_each_colon::parse;
		}
	};

	struct operation_loop_each_val :
	public with_kind<rk_none>,
	public check_none {
		static wtext name(state & st) { return L"t_loop_each_value_variable"; }

		static bool parse(state & st, t_tokens & toks, base_log * log) {
			bool ret = false;
			state tmp_st = st;
			if( is_name::parse(tmp_st, toks, log) ) {
				ret = true;
				wtext var_name(tmp_st.prev_str_, tmp_st.str_);
				bool is_by_ref = false;
				if( *tmp_st.str_ == L'&' ) {
					is_by_ref = true;
					++tmp_st.str_;
				}
				st.next_str(tmp_st.str_);
				toks.append(new tokens::token_loop_each_val(var_name, is_by_ref) );
			}
			return ret;
		}

		static void post_action(state & st, t_tokens & toks, base_log * log) {
			st.next_fn_ = rule_loop_each_after_val::parse;
		}
	};

	struct operation_loop_each_colon :
	public with_kind<rk_none>,
	public check_none,
	public is_char<L':'> {
		static wtext name(state & st) { return L"t_loop_each_colon"; }

		static void post_action(state & st, t_tokens & toks, base_log * log) {
			st.next_fn_ = rule_loop_each_val::parse;
		}
	};

	struct operation_loop_for :
	public with_kind<rk_operand>,
	public check_none,
	public is_keyword<operation_loop_for>,
	public pa_none {
		static wtext name(state & st) { return L"t_loop_for"; }

		static bool check_text(const wtext & tx, state & st, t_tokens & toks, base_log * log) {
			return text_compare(tx, L"`for");
		}

		static bool parse(state & st, t_tokens & toks, base_log * log);
	};

	struct operation_loop_for_key :
	public with_kind<rk_none>,
	public check_none,
	public is_name {
		static wtext name(state & st) { return L"t_loop_for_key_variable"; }

		static void post_action(state & st, t_tokens & toks, base_log * log) {
			toks.append(new tokens::token_loop_for_key(st.liner_.get_pos(st.prev_str_), wtext(st.prev_str_, st.str_) ) );
			st.next_fn_ = rule_loop_for_colon::parse;
		}
	};

	struct operation_loop_for_val :
	public with_kind<rk_none>,
	public check_none,
	public is_name {
		static wtext name(state & st) { return L"t_loop_for_value_variable"; }

		static void post_action(state & st, t_tokens & toks, base_log * log) {
			toks.append(new tokens::token_loop_for_val(wtext(st.prev_str_, st.str_) ) );
			st.next_fn_ = rule_loop_for_after_val::parse;
		}
	};

	struct operation_loop_for_colon :
	public with_kind<rk_none>,
	public check_none,
	public is_char<L':'> {
		static wtext name(state & st) { return L"t_loop_for_colon"; }

		static void post_action(state & st, t_tokens & toks, base_log * log) {
			st.next_fn_ = rule_loop_for_val::parse;
		}
	};

	struct t_operator :
	public with_kind<rk_operator>,
	public check_none,
	public rule_alt<false, false, t_operator_named, t_operator_std> {
		static wtext name(state & st) { return L"t_operator"; }

		static void post_action(state & st, t_tokens & toks, base_log * log) {
			st.next_fn_ = rule_expr::parse;
		}
	};

	struct t_prefix_operator :
	public with_kind<rk_prefix_operator>,
	public rule_alt<false, false, t_prefix_operator_named, t_prefix_operator_std>,
	public pa_add_flag<flag_was_prefix_operator> {
		static wtext name(state & st) { return L"t_prefix_operator"; }

		static bool check(state & st) {
			return
				st.nest_ == nest_parentheses &&
				!st.expressions_count_ &&
				check_flag_is_absent<flag_was_prefix_operator>::check(st)
			;
		}
	};

	struct t_operator_assign :
	public with_kind<rk_operator>,
	public check_none,
	public pa_done_good {
		static wtext name(state & st) { return L"t_operator_assign"; }

		static bool parse(state & st, t_tokens & toks, base_log * log);
	};

	struct t_operator_assign_rt :
	public with_kind<rk_operator>,
	public check_none {
		static wtext name(state & st) { return L"t_operator_assign_to_right"; }

		static bool parse(state & st, t_tokens & toks, base_log * log);

		static void post_action(state & st, t_tokens & toks, base_log * log) {
			st.next_fn_ = rule_expr_after_assign_rt::parse;
		}
	};

	struct t_operator_pair :
	public with_kind<rk_operator>,
	public is_char<L':'> {
		static wtext name(state & st) { return L"t_colon"; }

		static bool check(state & st) {
			return st.nest_ == nest_map && check_flag_is_absent<flag_was_colon>::check(st);
		}

		static void post_action(state & st, t_tokens & toks, base_log * log);
	};

	struct fn_call_native :
	public with_kind<rk_fn_call>,
	public check_none,
	public pa_none {
		static wtext name(state & st) { return L"t_native_function_call"; }

		static bool parse(state & st, t_tokens & toks, base_log * log);
	};

	struct fn_call_global :
	public with_kind<rk_fn_call>,
	public check_none,
	public pa_none {
		static wtext name(state & st) { return L"t_global_function_call"; }

		static bool parse(state & st, t_tokens & toks, base_log * log);
	};

	struct separator :
	public with_kind<rk_separator>,
	public check_none,
	public is_char<L','> {
		static wtext name(state & st) { return L"t_comma"; }

		static void post_action(state & st, t_tokens & toks, base_log * log) {
			st.next_fn_ = rule_expr::parse;
		}
	};

	//

	template <bool Is_set, bool In_assign>
	struct wrap_dot {

		struct t_operator_dot :
		public with_kind<rk_operator>,
		public std::conditional_t<
			Is_set,
			check_kind<rk_operand_can_dot_set>,
			check_kind<rk_operand_can_dot_get>
		>,
		is_char<L'.'> {
			static wtext name(state & st) { return L"t_operator_dot"; }

			static void post_action(state & st, t_tokens & toks, base_log * log) {
				std::conditional_t<Is_set, for_dot_set, for_dot_get>::add_token(st, toks);
				st.next_fn_ = rule_expr_after_dot<Is_set, In_assign>::parse;
			}
		};

		struct operand_after_dot :
		public std::conditional_t<
			Is_set,
			with_kind<rk_operand_can_dot_set>,
			with_kind<rk_operand_can_dot_get>
		>,
		public check_none,
		public std::conditional_t<
			Is_set,
			rule_alt<false, false, lit_bool, lit_null, lit_int, lit_text_after_dot, operand_type>,
			rule_alt<false, false, lit_int, lit_text_after_dot, operand_type>
		> {
			static wtext name(state & st) {
				if constexpr( Is_set )
				return L"t_operand_key_after_dot_set";
				else
				return L"t_operand_key_after_dot_get";
			}

			static void post_action(state & st, t_tokens & toks, base_log * log) {
				if constexpr( In_assign ) {
					st.next_fn_ = rule_expr_assign_after_operand::parse;
				} else {
					st.next_fn_ = rule_expr_after_operand::parse;
				}
			}
		};

		struct operation_bracket :
		public std::conditional_t<
			Is_set,
			with_kind<rk_operand_can_dot_set>,
			with_kind<rk_operand_can_dot_get>
		>,
		public pa_none {
			static wtext name(state & st) { return L"t_operation_bracket"; }

			static bool check(state & st) {
				return !st.was_od_ && std::conditional_t<
					Is_set,
					check_kind<rk_operand_can_dot_set>,
					check_kind<rk_operand_can_dot_get>
				>::check(st);
			}

			static bool parse(state & st, t_tokens & toks, base_log * log) {
				if( *st.str_ != L'[' ) return false;
				std::conditional_t<Is_set, for_dot_set, for_dot_get>::add_bracket_begin(st, toks);
				const Char * str = st.str_ +1;
				info tmp_inf = { st.inf_->path_ };
				state tmp_st = {
					&tmp_inf,
					str,
					str,
					st.liner_,
					st.loop_depth_,
					rule_expr::parse,
					nest_bracket
				};
				base_rule::go(tmp_st, toks, log);
				bool ret = false;
				if( tmp_inf.good_ ) {
					for_dot_get::add_bracket_end(st, toks);
					st.next_str(tmp_st.str_);
					st.liner_ = tmp_st.liner_;
					ret = true;
				} else {
					st.done_ = true;
				}
				return ret;
			}
		};

	};

	// rules

	struct rule_file : public rule_alt<true, true, od, module_info_begin, fn_def, keyword_plain, end_file> {};
	struct rule_module_info : public rule_alt<true, true, module_name, od, end_module_info> {};

	struct rule_fn_arg_1_or_type : public rule_alt<true, true, od, fn_def_type, fn_def_arg<true> > {};
	template <bool Is_first>
	struct rule_fn_arg : public rule_alt<true, true, od, fn_def_arg<Is_first> > {};
	struct rule_fn_body : public rule_alt<true, true, od, fn_def_body> {};

	struct rule_expr : public rule_alt<true, true, od, end_expr, operation_assign, operand, t_prefix_operator> {};
	struct rule_expr_after_operand : public rule_alt<true, true,
		od,
		end_expr,
		separator,
		operation_loop_for,
		operation_loop_each,
		fn_call_native,
		fn_call_global,
		operation_assign,
		wrap_dot<false, false>::operation_bracket,
		wrap_dot<true, false>::operation_bracket,
		operand,
		wrap_dot<false, false>::t_operator_dot,
		wrap_dot<true, false>::t_operator_dot,
		t_operator_pair,
		t_operator_assign_rt,
		t_operator,
		operation_condition
	> {};
	struct rule_expr_after_assign_rt : public rule_alt<true, true, od, operand_target<false> > {};

	struct rule_expr_assign : public rule_alt<true, true, operand_target<true> > {};
	struct rule_expr_assign_after_operand : public rule_alt<true, true,
		od,
		wrap_dot<true, true>::t_operator_dot,
		wrap_dot<true, true>::operation_bracket,
		t_operator_assign
	> {};

	template <bool Is_set, bool In_assign>
	struct rule_expr_after_dot : public rule_alt<true, true, od, typename wrap_dot<Is_set, In_assign>::operand_after_dot> {};

	struct rule_loop_each_key : public rule_alt<true, true, od, operation_loop_each_key_sign, operation_loop_each_key> {};
	struct rule_loop_each_colon : public rule_alt<true, true, od, operation_loop_each_colon> {};
	struct rule_loop_each_val : public rule_alt<true, true, od, operation_loop_each_val_sign, operation_loop_each_val> {};
	struct rule_loop_each_after_val : public rule_alt<true, true, od, end_expr> {};

	struct rule_loop_for_key : public rule_alt<true, true, od, operation_loop_for_key> {};
	struct rule_loop_for_colon : public rule_alt<true, true, od, operation_loop_for_colon> {};
	struct rule_loop_for_val : public rule_alt<true, true, od, operation_loop_for_val> {};
	struct rule_loop_for_after_val : public rule_alt<true, true, od, end_expr> {};

};

} // ns
} // ns
