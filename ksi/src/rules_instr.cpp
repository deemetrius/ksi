#include "rules.ksi.h"
#include "instr.h"

namespace ksi {
namespace rules {

struct action_loop {
	// returns true on error
	inline static bool process_block(
		state & st, t_tokens & toks, base_log * log,
		const Char *& str, liner & lnr, Char & last_char,
		info & tmp_inf, id loop_depth, n_expr_nest nest, uid flags
	) {
		state tmp_st = {
			&tmp_inf,
			str,
			str,
			lnr,
			loop_depth,
			hive::rule_expr::parse,
			nest,
			flags
		};
		base_rule::go(tmp_st, toks, log);
		if( tmp_inf.good_ ) {
			str = tmp_st.str_;
			lnr = tmp_st.liner_;
			last_char = str[-1];
		} else {
			st.done_ = true;
			return true;
		}
		return false;
	}
};

bool hive::operand_loop_while::parse(state & st, t_tokens & toks, base_log * log) {
	const Char * str;
	//
	{
		state tmp_st = st;
		if( !self_base_is_keyword::parse(tmp_st, toks, log) ) return false;
		str = tmp_st.str_;
	}

	const mod::instr_type * i_type = mod::instructions::get_while_pre();
	if( *str == L':' ) {
		i_type = mod::instructions::get_while_post();
		++str;
	}

	hive::operand::maybe_next_expr(st, toks, log);
	toks.append( new tokens::token_loop_while_begin(st.liner_.get_pos(st.str_), i_type) );

	info tmp_inf = { st.inf_->path_ };
	liner lnr = st.liner_;
	id loop_depth = st.loop_depth_ +1;
	Char last_char;

	if( rules::action_loop::process_block(
		st, toks, log,
		str, lnr, last_char,
		tmp_inf, loop_depth, nest_while_condition, 0
	) ) return false;
	tmp_inf.good_ = false;

	if( last_char == L'\\' ) {
		toks.append( new tokens::token_loop_while_also_block() );
		if( rules::action_loop::process_block(
			st, toks, log,
			str, lnr, last_char,
			tmp_inf, loop_depth, nest_while_condition, flag_second_part
		) ) return false;
		tmp_inf.good_ = false;
	}

	toks.append( new tokens::token_loop_while_body() );
	if( rules::action_loop::process_block(
		st, toks, log,
		str, lnr, last_char,
		tmp_inf, loop_depth, nest_loop_body, 0
	) ) return false;
	tmp_inf.good_ = false;

	if( last_char == L'\\' ) {
		toks.append( new tokens::token_loop_while_body_rest() );
		if( rules::action_loop::process_block(
			st, toks, log,
			str, lnr, last_char,
			tmp_inf, loop_depth, nest_loop_body, flag_second_part
		) ) return false;
	}

	toks.append( new tokens::token_loop_while_end() );
	st.next_str(str);
	st.liner_ = lnr;

	return true;
}

} // ns
} // ns
