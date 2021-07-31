#include "tokens.inc.h"
#include "ast.h"
#include "instr.h"

namespace ksi {
namespace tokens {

void token_set_module_name::prepare(space * spc, ast::prepare_data * pd, base_log * log) {
	pd->mod_.h_->name_ = name_;
}

void token_plain::perform(space * spc, ast::prepare_data * pd, base_log * log) {
	pd->begin_plain(pos_);
}
void token_fn_begin::perform(space * spc, ast::prepare_data * pd, base_log * log) {
	pd->begin_fn_body(pos_, fn_name_);
	//ast::body * bd = pd->body_.h_;
	//bd->name_ = fn_name_;
}
void token_fn_end::perform(space * spc, ast::prepare_data * pd, base_log * log) {
	pd->end_fn_body();
}
void token_fn_set_overload::perform(space * spc, ast::prepare_data * pd, base_log * log) {
	if(
		var::base_type::type_config::map_types::t_res_node
		res = var::hcfg->tc->map_types_.find_node(type_name_)
	) {
		ast::body * bd = pd->body_.h_;
		bd->var_type_ = res.pos_->val_;
		if( is_static_ ) {
			bd->kind_ = mod::fk_static;
			bd->adder_ = mod::fn_traits<mod::fk_static>::reg;
		} else {
			bd->kind_ = mod::fk_non_static;
			bd->adder_ = mod::fn_traits<mod::fk_non_static>::reg;
		}
	} else {
		log->add({
			ex::implode({L"deduce error: Unknown type: $", type_name_}),
			pd->mod_.h_->path_,
			pos_
		});
		pd->error_count_ += 1;
	}
}

struct helper_fn_add_arg {
	template <bool Is_first>
	static inline wtext number() {
		if constexpr( Is_first )
		return L"1";
		else
		return L"2";
	}

	static inline void put_error(const wtext & msg, base_token_fn_add_arg * tok, ast::prepare_data * pd, base_log * log) {
		log->add({ msg, pd->mod_.h_->path_, tok->pos_ });
		++pd->error_count_;
	}

	template <bool Is_first>
	static inline void inner_perform(base_token_fn_add_arg * tok, space * spc, ast::prepare_data * pd, base_log * log) {
		bool no = false;
		wtext n = number<Is_first>();
		ast::body * bd = pd->body_.h_;
		mod::fn_body * fnb = bd->fn_body_.h_;
		if( tok->arg_name_ == wtext(L"ret") ) {
			put_error(
				ex::implode({ L"deduce error: fn arg ", n, L" can't be \"ret\"." }),
				tok, pd, log
			);
			no = true;
		} else if( tok->arg_name_ == wtext(L"_") ) {
			put_error(
				ex::implode({ L"deduce error: fn arg ", n, L" can't be \"_\"." }),
				tok, pd, log
			);
			no = true;
		} else if constexpr( !Is_first ) {
			mod::fn_body::t_var_names::cnode * arg_1 = static_cast<mod::fn_body::t_var_names::cnode *>(
				fnb->var_names_.items_.zero_.next_->next_
			);
			if( tok->arg_name_ == arg_1->key_ ) {
				put_error(
					ex::implode({ L"deduce error: fn arg 2 should differ from arg 1: \"", tok->arg_name_, L"\"." }),
					tok, pd, log
				);
				no = true;
			}
		}
		fnb->reg_var(no ? n : tok->arg_name_);
		if constexpr( !Is_first )
		fnb->reg_var(L"_");
	}
};

void base_token_fn_add_arg::perform_1(space * spc, ast::prepare_data * pd, base_log * log) {
	helper_fn_add_arg::inner_perform<true>(this, spc, pd, log);
}
void base_token_fn_add_arg::perform_2(space * spc, ast::prepare_data * pd, base_log * log) {
	helper_fn_add_arg::inner_perform<false>(this, spc, pd, log);
}

//

void token_put_null::perform(space * spc, ast::prepare_data * pd, base_log * log) {
	ast::tree * tr = pd->current_tree();
	ast::tree::add_leaf(tr, new ast::node{
		ast::actions::info_leaf(),
		{mod::instructions::get_lit_null(), {pos_} }
	});
}

void token_put_bool::perform(space * spc, ast::prepare_data * pd, base_log * log) {
	ast::tree * tr = pd->current_tree();
	ast::tree::add_leaf(tr, new ast::node{
		ast::actions::info_leaf(),
		{mod::instructions::get_lit_bool(), {pos_, val_} }
	});
}

void token_put_int::perform(space * spc, ast::prepare_data * pd, base_log * log) {
	ast::tree * tr = pd->current_tree();
	ast::tree::add_leaf(tr, new ast::node{
		ast::actions::info_leaf(),
		{mod::instructions::get_lit_int(), {pos_, val_} }
	});
}

void token_put_float::perform(space * spc, ast::prepare_data * pd, base_log * log) {
	mod::module * md = pd->mod_.h_;
	id data = md->lit_.count_;
	md->lit_.append(val_);
	ast::tree * tr = pd->current_tree();
	ast::tree::add_leaf(tr, new ast::node{
		ast::actions::info_leaf(),
		{mod::instructions::get_literal(), {pos_, data} }
	});
}

void token_put_text::perform(space * spc, ast::prepare_data * pd, base_log * log) {
	mod::module * md = pd->mod_.h_;
	id data = md->lit_.count_;
	md->lit_.append(val_);
	ast::tree * tr = pd->current_tree();
	ast::tree::add_leaf(tr, new ast::node{
		ast::actions::info_leaf(),
		{mod::instructions::get_literal(), {pos_, data} }
	});
}

void token_put_type::perform(space * spc, ast::prepare_data * pd, base_log * log) {
	id num = 0;
	if( ex::id_search_res res = spc->types_.find_pos(type_name_) )
	num = res.pos_;
	else {
		log->add({
			ex::implode({L"deduce error: Unknown type: $", type_name_}),
			pd->mod_.h_->path_,
			pos_
		});
		pd->error_count_ += 1;
	}

	ast::tree * tr = pd->current_tree();
	ast::tree::add_leaf(tr, new ast::node{
		ast::actions::info_leaf(),
		{mod::instructions::get_type(), {pos_, num} }
	});

}

void token_put_var::perform(space * spc, ast::prepare_data * pd, base_log * log) {
	mod::fn_body * fnb = pd->body_.h_->fn_body_.h_;
	id data = fnb->reg_var(var_name_);
	ast::tree * tr = pd->current_tree();
	ast::tree::add_leaf(tr, new ast::node{
		ast::actions::info_leaf(),
		{mod::instructions::get_var_get(), {pos_, data} }
	});
}

void token_put_var_link::perform(space * spc, ast::prepare_data * pd, base_log * log) {
	mod::fn_body * fnb = pd->body_.h_->fn_body_.h_;
	id data = fnb->reg_var(var_name_);
	ast::tree * tr = pd->current_tree();
	ast::tree::add_leaf(tr, new ast::node{
		ast::actions::info_leaf(),
		{mod::instructions::get_var_target(), {pos_, data} }
	});
}

void token_scope_begin::perform(space * spc, ast::prepare_data * pd, base_log * log) {
	ast::tree * tr = pd->current_tree();
	ast::node * nd_scope = new ast::node{
		ast::actions::info_leaf(ast::actions::do_leaf_scope),
		{mod::instructions::get_lit_null(), {pos_} }
	};
	ast::tree::add_leaf(tr, nd_scope);
	ast::body * bd = pd->body_.h_;
	nd_scope->info_.data_ = bd->add_scope_pos( ast::scope::make_parentheses_scope() );
}

void token_scope_end::perform(space * spc, ast::prepare_data * pd, base_log * log) {
	ast::body * bd = pd->body_.h_;
	bd->del_scope_pos();
}

void token_array_begin::perform(space * spc, ast::prepare_data * pd, base_log * log) {
	ast::tree * tr = pd->current_tree();
	ast::node * nd_scope = new ast::node{
		ast::actions::info_leaf(ast::actions::do_leaf_array),
		{mod::instructions::get_array_put(), {pos_} }
	};
	ast::tree::add_leaf(tr, nd_scope);
	ast::body * bd = pd->body_.h_;
	nd_scope->info_.data_ = bd->add_scope_pos( ast::scope::make_array_scope() );
}

void token_map_begin::perform(space * spc, ast::prepare_data * pd, base_log * log) {
	ast::tree * tr = pd->current_tree();
	ast::node * nd_scope = new ast::node{
		ast::actions::info_leaf(ast::actions::do_leaf_array),
		{mod::instructions::get_map_put(), {pos_} }
	};
	ast::tree::add_leaf(tr, nd_scope);
	ast::body * bd = pd->body_.h_;
	nd_scope->info_.data_ = bd->add_scope_pos( ast::scope::make_map_scope() );
}

void token_bracket_begin::perform(space * spc, ast::prepare_data * pd, base_log * log) {
	ast::tree * tr = pd->current_tree();
	tr->add_op(pos_, op_);
	ast::node * nd_scope = new ast::node{
		ast::actions::info_leaf(ast::actions::do_leaf_scope, ast::nk_bracket_args),
		{mod::instructions::get_lit_null(), {pos_} }
	};
	ast::tree::add_leaf(tr, nd_scope);
	ast::body * bd = pd->body_.h_;
	nd_scope->info_.data_ = bd->add_scope_pos( ast::scope::make_bracket_scope() );
}

void token_then::perform(space * spc, ast::prepare_data * pd, base_log * log) {
	ast::tree * tr = pd->current_tree();
	tr->add_op(pos_, op_);
	ast::node * nd_scope = new ast::node{
		ast::actions::info_leaf(ast::actions::do_leaf_then),
		{mod::instructions::get_then(), {pos_} }
	};
	ast::tree::add_leaf(tr, nd_scope);
	ast::body * bd = pd->body_.h_;
	nd_scope->info_.data_ = bd->add_scope_pos( ast::scope::make_parentheses_scope() );
}

void token_else::perform(space * spc, ast::prepare_data * pd, base_log * log) {
	ast::body * bd = pd->body_.h_;
	bd->del_scope_pos();
	ast::tree * tr = pd->current_tree();
	ast::node * nd_scope = tr->leafs_.last(0);
	nd_scope->info_.extra_ = bd->add_scope_pos( ast::scope::make_parentheses_scope() );
	ast::actions::turn_leaf_then_to_else(nd_scope);
}

void token_condition_end::perform(space * spc, ast::prepare_data * pd, base_log * log) {
	ast::body * bd = pd->body_.h_;
	bd->del_scope_pos(depth_);
}

void token_loop_while_begin::perform(space * spc, ast::prepare_data * pd, base_log * log) {
	ast::tree * tr = pd->current_tree();
	ast::node * nd_scope = new ast::node{
		ast::actions::info_leaf(ast::actions::do_leaf_while),
		{i_type_, {pos_} }
	};
	ast::tree::add_leaf(tr, nd_scope);
	ast::body * bd = pd->body_.h_;
	nd_scope->info_.data_ = bd->add_scope_pos( ast::scope::make_parentheses_scope() );
}

void token_loop_while_also_block::perform(space * spc, ast::prepare_data * pd, base_log * log) {
	ast::body * bd = pd->body_.h_;
	bd->del_scope_pos();
	ast::tree * tr = pd->current_tree();
	ast::node * nd_scope = tr->leafs_.last(0);
	nd_scope->info_.also_ = bd->add_scope_pos( ast::scope::make_function_scope() );
}

void token_loop_while_body::perform(space * spc, ast::prepare_data * pd, base_log * log) {
	ast::body * bd = pd->body_.h_;
	bd->del_scope_pos();
	ast::tree * tr = pd->current_tree();
	ast::node * nd_scope = tr->leafs_.last(0);
	nd_scope->info_.extra_ = bd->add_scope_pos( ast::scope::make_function_scope() );
}

void token_loop_while_body_rest::perform(space * spc, ast::prepare_data * pd, base_log * log) {
	ast::body * bd = pd->body_.h_;
	bd->del_scope_pos();
	ast::tree * tr = pd->current_tree();
	ast::node * nd_scope = tr->leafs_.last(0);
	nd_scope->info_.mod_ = bd->add_scope_pos( ast::scope::make_function_scope() );
}

//

void token_loop_each_begin::perform(space * spc, ast::prepare_data * pd, base_log * log) {
	ast::tree * tr = pd->current_tree();
	tr->add_op(pos_, ast::actions::op_each() );
}

void token_loop_each_order::perform(space * spc, ast::prepare_data * pd, base_log * log) {
	ast::tree * tr = pd->current_tree();
	ast::node * nd_each = tr->nodes_.last(0);
	nd_each->instr_.params_.data_ = order_;
}

void token_loop_each_key::perform(space * spc, ast::prepare_data * pd, base_log * log) {
	mod::fn_body * fnb = pd->body_.h_->fn_body_.h_;
	id data = fnb->reg_var(var_name_);
	ast::tree * tr = pd->current_tree();
	ast::tree::add_leaf(tr, new ast::node{
		ast::actions::info_leaf(ast::actions::do_leaf_none),
		{mod::instructions::get_each_vars(), {pos_, data} }
	});
}

void token_loop_each_val::perform(space * spc, ast::prepare_data * pd, base_log * log) {
	ast::tree * tr = pd->current_tree();
	ast::node * nd_each_vars = tr->leafs_.last(0);
	mod::fn_body * fnb = pd->body_.h_->fn_body_.h_;
	nd_each_vars->instr_.params_.extra_ = fnb->reg_var(var_name_);
	if( is_by_ref_ )
	nd_each_vars->instr_.type_ = mod::instructions::get_each_vars_ref();
}

void token_loop_each_also_block::perform(space * spc, ast::prepare_data * pd, base_log * log) {
	ast::body * bd = pd->body_.h_;
	ast::tree * tr = pd->current_tree();
	ast::node * nd_scope = tr->nodes_.last(0);
	nd_scope->info_.also_ = bd->add_scope_pos( ast::scope::make_function_scope() );
}

void token_loop_each_body::perform(space * spc, ast::prepare_data * pd, base_log * log) {
	ast::body * bd = pd->body_.h_;
	if( was_also_block_ )
	bd->del_scope_pos();
	ast::tree * tr = pd->current_tree();
	ast::node * nd_scope = tr->nodes_.last(0);
	nd_scope->info_.extra_ = bd->add_scope_pos( ast::scope::make_function_scope() );
}

void token_loop_each_body_rest::perform(space * spc, ast::prepare_data * pd, base_log * log) {
	ast::body * bd = pd->body_.h_;
	bd->del_scope_pos();
	ast::tree * tr = pd->current_tree();
	ast::node * nd_scope = tr->nodes_.last(0);
	nd_scope->info_.mod_ = bd->add_scope_pos( ast::scope::make_function_scope() );
}

//

void token_loop_for_begin::perform(space * spc, ast::prepare_data * pd, base_log * log) {
	ast::tree * tr = pd->current_tree();
	tr->add_op(pos_, ast::actions::op_for() );
}

void token_loop_for_val::perform(space * spc, ast::prepare_data * pd, base_log * log) {
	ast::tree * tr = pd->current_tree();
	ast::node * nd_each_vars = tr->leafs_.last(0);
	mod::fn_body * fnb = pd->body_.h_->fn_body_.h_;
	nd_each_vars->instr_.params_.extra_ = fnb->reg_var(var_name_);
}

//

void token_kw_next::perform(space * spc, ast::prepare_data * pd, base_log * log) {
	ast::tree * tr = pd->current_tree();
	ast::tree::add_leaf(tr, new ast::node{
		ast::actions::info_leaf(),
		{i_type_, {pos_, depth_} }
	});
}

void token_kw_return::perform(space * spc, ast::prepare_data * pd, base_log * log) {
	ast::tree * tr = pd->current_tree();
	ast::tree::add_leaf(tr, new ast::node{
		ast::actions::info_leaf(),
		{mod::instructions::get_kw_return(), {pos_} }
	});
}

//

void token_next_expr::perform(space * spc, ast::prepare_data * pd, base_log * log) {
	ast::body * bd = pd->body_.h_;
	bd->current_scope()->add_tree();
}

void token_add_op::perform(space * spc, ast::prepare_data * pd, base_log * log) {
	pd->current_tree()->add_op(pos_, op_);
}

void token_add_fn_native::perform(space * spc, ast::prepare_data * pd, base_log * log) {
	id num = 0;
	if( ex::id_search_res res = ksi::var::hcfg->native_->fn_map_.find_pos(fn_name_) )
	num = res.pos_;
	else {
		log->add({
			ex::implode({L"deduce error: Unknown native function: ", fn_name_}),
			pd->mod_.h_->path_,
			pos_
		});
		pd->error_count_ += 1;
	}

	ast::tree * tr = pd->current_tree();
	ast::tree::add_node_assoc_left(tr, new ast::node{
		{ast::actions::do_node_assoc_left, ast::prec_plus, ast::prec_plus},
		{(is_bk_ ?
			mod::instructions::get_fn_native_call_2_bk() :
			mod::instructions::get_fn_native_call_2()
		), {pos_, num} }
	});
}

void token_add_fn_global::perform(space * spc, ast::prepare_data * pd, base_log * log) {
	ast::body * bd = pd->body_.h_;
	id n_place = pd->fn_call_places_.place_add(fn_name_, bd->fn_body_.h_, is_bk_, pos_, pd->mod_.h_);
	ast::tree * tr = pd->current_tree();
	ast::tree::add_node_assoc_left(tr, new ast::node{
		{ast::actions::do_fn_global_call, ast::prec_plus, ast::prec_plus, ast::nk_not_set, n_place},
		{mod::instructions::get_fn_native_call_2(), {pos_, 0} }
	});
}

} // ns
} // ns
