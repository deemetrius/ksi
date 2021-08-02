#include "rules.ksi.h"
#include "ast.h"

namespace ksi {
namespace rules {

struct helper_operator {
	template <class Token = tokens::token_add_op>
	static bool op_iter_parse(const ast::array_iter<ast::op_info> & iter, state & st, t_tokens & toks, base_log * log) {
		bool ret = false;
		id len;
		for( const ast::op_info & it : iter )
		if( !std::wcsncmp(st.str_, it.str_.h_->cs_, len = it.str_.h_->len_) ) {
			ret = true;
			toks.append(new Token(st.liner_.get_pos(st.str_), &it) );
			st.next_str(st.str_ + len);
			break;
		}
		return ret;
	}
};

bool t_operator_std::parse(state & st, t_tokens & toks, base_log * log) {
	return helper_operator::op_iter_parse(ast::actions::iter_op(), st, toks, log);
}

bool t_prefix_operator_std::parse(state & st, t_tokens & toks, base_log * log) {
	return helper_operator::op_iter_parse<tokens::token_set_prefix_operator>(ast::actions::iter_prefix_op(), st, toks, log);
}

bool t_operator_named::check_text(const wtext & tx, state & st, t_tokens & toks, base_log * log) {
	bool ret = false;
	if( ast::actions::t_map_named_ops::t_res_node res = ast::actions::map_named_ops().find_node(tx) ) {
		ret = true;
		toks.append(new tokens::token_add_op(st.liner_.get_pos(st.str_), &res.pos_->val_) );
	}
	return ret;
}

bool t_prefix_operator_named::check_text(const wtext & tx, state & st, t_tokens & toks, base_log * log) {
	bool ret = false;
	if( ast::actions::t_map_named_ops::t_res_node res = ast::actions::map_prefix_named_ops().find_node(tx) ) {
		ret = true;
		toks.append(new tokens::token_set_prefix_operator(st.liner_.get_pos(st.str_), &res.pos_->val_) );
	}
	return ret;
}

bool hive::t_operator_assign::parse(state & st, t_tokens & toks, base_log * log) {
	bool ret = false;
	id len;
	const Char * end;
	for( const ast::op_info & it : ast::actions::iter_op_assign() )
	if( !std::wcsncmp(st.str_, it.str_.h_->cs_, len = it.str_.h_->len_) ) {
		end = st.str_ + len;
		if( *end != L'=' && *end != L'>' ) {
			ret = true;
			toks.append(new tokens::token_add_op(st.liner_.get_pos(st.str_), &it) );
			st.next_str(end);
		}
		break;
	}
	return ret;
}

bool hive::t_operator_assign_rt::parse(state & st, t_tokens & toks, base_log * log) {
	return helper_operator::op_iter_parse(ast::actions::iter_op_assign_rt(), st, toks, log);
}

bool hive::operation_condition::parse(state & st, t_tokens & toks, base_log * log) {
	if( *st.str_ != L'?' ) return false;
	toks.append(new tokens::token_then(st.liner_.get_pos(st.str_), ast::actions::op_then() ) );
	info tmp_inf = { st.inf_->path_ };
	const Char * str = st.str_ +1;
	liner lnr = st.liner_;
	uid flags = 0;
	id depth = 1;
	bool ret = false;
	do {
		state tmp_st = {
			&tmp_inf,
			str,
			str,
			lnr,
			st.loop_depth_,
			rule_expr::parse,
			nest_condition,
			flags
		};
		base_rule::go(tmp_st, toks, log);
		if( tmp_inf.good_ ) {
			str = tmp_st.str_;
			lnr = tmp_st.liner_;
			Char last_char = str[-1];
			if( last_char == L'?' ) {
				toks.append(new tokens::token_then(st.liner_.get_pos(str -1), ast::actions::op_then() ) );
				flags = 0;
				++depth;
			} else if( last_char == L'|' ) {
				toks.append(new tokens::token_else() );
				flags = flag_condition_else;
			} else if( last_char == L';' ) {
				toks.append(new tokens::token_condition_end(depth) );
				ret = true;
				break;
			}
			tmp_inf.good_ = false;
		} else {
			st.done_ = true;
			break;
		}
	} while( true );
	if( ret ) {
		st.next_str(str);
		st.liner_ = lnr;
	}
	return ret;
}

void hive::t_operator_pair::post_action(state & st, t_tokens & toks, base_log * log) {
	pa_add_flag<flag_was_colon>::post_action(st, toks, log);
	toks.append(new tokens::token_add_op(st.liner_.get_pos(st.prev_str_), ast::actions::op_colon() ) );
	st.next_fn_ = rule_expr::parse;
}

//

void for_dot_get::add_token(state & st, t_tokens & toks) {
	toks.append(new tokens::token_add_op(st.liner_.get_pos(st.prev_str_), ast::actions::op_dot_get() ) );
}

void for_dot_set::add_token(state & st, t_tokens & toks) {
	toks.append(new tokens::token_add_op(st.liner_.get_pos(st.prev_str_), ast::actions::op_dot_set() ) );
}

//

void for_dot_get::add_bracket_begin(state & st, t_tokens & toks) {
	toks.append(new tokens::token_bracket_begin(st.liner_.get_pos(st.prev_str_), ast::actions::op_bracket_get() ) );
}

void for_dot_set::add_bracket_begin(state & st, t_tokens & toks) {
	toks.append(new tokens::token_bracket_begin(st.liner_.get_pos(st.prev_str_), ast::actions::op_bracket_set() ) );
}

} // ns
} // ns
