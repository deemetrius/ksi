#include "ast.h"
#include "instr.h"

namespace ksi {
namespace ast {

tree::tree() {
	nodes_.append(new node{actions::info_root() });
}

void scope::perform(prepare_data * pd) {
	if( trees_.count_ ) {
		tree
			* first = *trees_.items_,
			* last = trees_.last(0)
		;
		node * root;
		body * bd = pd->body_.h_;
		mod::instr * i_type, i_tmp;
		for( tree * it : trees_ ) {
			root = *it->nodes_.items_;
			root->info_.action_(pd, it, root, root);
			i_type = (it == last ? &i_last_ :
				(it == first ? &i_first_ : &i_step_)
			);
			if( it->leafs_.count_ )
			i_type->params_.pos_ = it->leafs_.items_[0]->instr_.params_.pos_;

			if( root->rt_ && root->rt_->info_.kind_ == nk_pair ) {
				i_tmp = *i_type;
				i_tmp.type_ = mod::instructions::get_map_append_2();
				i_type = &i_tmp;
			}

			bd->current_side()->add_instr(*i_type);
		}
	}
}

scope * scope::make_function_scope() {
	return new scope{
		{ mod::instructions::get_next_expr(),	{{0, 0}} },
		{ mod::instructions::get_next_expr(),	{{0, 0}} },
		{ mod::instructions::get_next_expr(),	{{0, 0}} }
	};
}
scope * scope::make_parentheses_scope() {
	return new scope{
		{ mod::instructions::get_next_expr(),	{{0, 0}} },
		{ mod::instructions::get_next_expr(),	{{0, 0}} },
		{ mod::instructions::get_nothing(),		{{0, 0}} }
	};
}
scope * scope::make_array_scope() {
	return new scope{
		{ mod::instructions::get_array_append(), {{0, 0}} },
		{ mod::instructions::get_array_append(), {{0, 0}} },
		{ mod::instructions::get_array_append(), {{0, 0}} }
	};
}
scope * scope::make_map_scope() {
	return new scope{
		{ mod::instructions::get_map_append_1(), {{0, 0}} },
		{ mod::instructions::get_map_append_1(), {{0, 0}} },
		{ mod::instructions::get_map_append_1(), {{0, 0}} }
	};
}
scope * scope::make_bracket_scope() {
	return new scope{
		{ mod::instructions::get_array_append(),	{{0, 0}} },
		{ mod::instructions::get_array_append(),	{{0, 0}} },
		{ mod::instructions::get_nothing(),			{{0, 0}} }
	};
}

void fn_info::put_fn(prepare_data * pd, space * spc, base_log * log) {
	mod::fn_body * fnb = fn_body_.h_;
	also::t_pos pos = fnb->pos_;
	mod::fn_reg_result res = adder_(spc, name_, var_type_, fnb);
	fn_body_.detach();
	if( res ) {
		pd->fn_call_places_.actual_not(fnb);
		wtext msg = kind_ == mod::fk_common ?
			ex::implode({
				L"deduce notice: Function ", name_, L" was already commonly overloaded at line ",
				ex::to_wtext(res.pos_->pos_.line_), L" char ", ex::to_wtext(res.pos_->pos_.col_)
			})
		: ( kind_ == mod::fk_non_static ?
			ex::implode({
				L"deduce notice: Function ", name_, L" was already overloaded for type $",
				var::type_text::get_text(var_type_->name_), L" at line ",
				ex::to_wtext(res.pos_->pos_.line_), L" char ", ex::to_wtext(res.pos_->pos_.col_)
			})
		:
			ex::implode({
				L"deduce notice: Function ", name_, L" was already statically overloaded for type $",
				var::type_text::get_text(var_type_->name_), L" at line ",
				ex::to_wtext(res.pos_->pos_.line_), L" char ", ex::to_wtext(res.pos_->pos_.col_)
			})
		);
		log->add({ msg, res.pos_->mod_->path_, pos });
	}
}

void fn_call_places::check_places(prepare_data * pd, base_log * log) {
	for( place & it : call_places_ )
	if( !global_check(it.fn_name_) ) {
		log->add({
			ex::implode({L"deduce error: Call to undefined function ", it.fn_name_}),
			it.mod_->path_, it.call_pos_
		});
		++pd->error_count_;
	}
}

void fn_call_places::update_instructions(space * spc, base_log * log) {
	for( place & it : call_places_ )
	if( *it.actual_ ) {
		if( ex::id_search_res res = spc->fn_map_.find_pos(it.fn_name_) ) {
			mod::instr * ins = it.side_->instructions_.items_ + it.i_pos_;
			ins->type_ = it.is_bk_ ?
				mod::instructions::get_fn_global_call_2_bk() :
				mod::instructions::get_fn_global_call_2()
			;
			ins->params_.data_ = res.pos_;
		} else {
			log->add({
				ex::implode({L"internal error: Calling to the function that is not found in space ", it.fn_name_}),
				it.mod_->path_, it.call_pos_
			});
		}
	}
}

void actions::do_concat_assoc_left(prepare_data * pd, tree * tr, node * parent, node * nd) {
	if( parent->info_.kind_ != nk_concat ) {
		id depth = calc_concat_depth(nd);
		if( depth > 1 ) {
			mod::side * sd = pd->body_.h_->current_side();
			sd->add_instr({
				mod::instructions::get_for_implode_make(),
				{nd->instr_.params_.pos_, depth +1}
			});
			// left
			nd->lt_->info_.action_(pd, tr, nd, nd->lt_);
			// right
			nd->rt_->info_.action_(pd, tr, nd, nd->rt_);
			sd->add_instr({
				mod::instructions::get_for_implode_add(),
				{nd->instr_.params_.pos_, 1}
			});
			//
			sd->add_instr({
				mod::instructions::get_for_implode_go(),
				{nd->instr_.params_.pos_}
			});
		} else {
			do_node_assoc_left(pd, tr, parent, nd);
		}
	} else {
		mod::side * sd = pd->body_.h_->current_side();
		// left
		nd->lt_->info_.action_(pd, tr, nd, nd->lt_);
		if( nd->lt_->info_.kind_ != nk_concat )
		sd->add_instr({
			mod::instructions::get_for_implode_add(),
			{nd->instr_.params_.pos_, 0}
		});
		// right
		nd->rt_->info_.action_(pd, tr, nd, nd->rt_);
		sd->add_instr({
			mod::instructions::get_for_implode_add(),
			{nd->instr_.params_.pos_, 1}
		});
	}
}

void actions::do_concat_assoc_right(prepare_data * pd, tree * tr, node * parent, node * nd) {
	if( parent->info_.kind_ != nk_concat ) {
		id depth = calc_concat_depth(nd);
		if( depth > 1 ) {
			mod::side * sd = pd->body_.h_->current_side();
			sd->add_instr({
				mod::instructions::get_for_implode_make(),
				{nd->instr_.params_.pos_, depth +1}
			});
			// right
			nd->rt_->info_.action_(pd, tr, nd, nd->rt_);
			sd->add_instr({
				mod::instructions::get_for_implode_add(),
				{nd->instr_.params_.pos_, 1}
			});
			// left
			nd->lt_->info_.action_(pd, tr, nd, nd->lt_);
			//
			sd->add_instr({
				mod::instructions::get_for_implode_go(),
				{nd->instr_.params_.pos_}
			});
		} else {
			do_node_assoc_right(pd, tr, parent, nd);
		}
	} else {
		mod::side * sd = pd->body_.h_->current_side();
		// right
		nd->rt_->info_.action_(pd, tr, nd, nd->rt_);
		sd->add_instr({
			mod::instructions::get_for_implode_add(),
			{nd->instr_.params_.pos_, 1}
		});
		// left
		nd->lt_->info_.action_(pd, tr, nd, nd->lt_);
		if( nd->lt_->info_.kind_ != nk_concat )
		sd->add_instr({
			mod::instructions::get_for_implode_add(),
			{nd->instr_.params_.pos_, 0}
		});
	}
}

struct action_condition {
	template <bool Null_on_empty, bool Need_add = false>
	inline static id scope_to_side(prepare_data * pd, body * bd, node * nd, id scope_pos, node * nd_add = nullptr) {
		id ret = bd->add_side_pos();
		if constexpr( Need_add )
		actions::node_add_instr(pd, nd_add);
		//
		scope * sc = bd->scopes_.items_[scope_pos];
		if( sc->trees_.count_ )
		sc->perform(pd);
		else if constexpr( Null_on_empty ) {
			mod::side * sd = bd->current_side();
			sd->add_instr({
				mod::instructions::get_lit_null(),
				{nd->instr_.params_.pos_}
			});
		}
		bd->del_side_pos();
		return ret;
	}
};

// each
void actions::do_each_assoc_left(prepare_data * pd, tree * tr, node * parent, node * nd) {
	nd->lt_->info_.action_(pd, tr, nd, nd->lt_);
	body * bd = pd->body_.h_;
	// primary loop body
	nd->instr_.params_.extra_ = action_condition::scope_to_side<false, true>(pd, bd, nd, nd->info_.extra_, nd->rt_);
	// rest loop body, maybe
	if( nd->info_.mod_ )
	nd->instr_.params_.mod_ = action_condition::scope_to_side<false, true>(pd, bd, nd, nd->info_.mod_, nd->rt_);
	// additional block, maybe
	if( nd->info_.also_ )
	nd->instr_.params_.also_ = action_condition::scope_to_side<false>(pd, bd, nd, nd->info_.also_);
	// current
	node_add_instr(pd, nd);
}

// ?
void actions::do_leaf_then(prepare_data * pd, tree * tr, node * parent, node * nd) {
	// prepare
	body * bd = pd->body_.h_;
	// then part
	nd->instr_.params_.data_ = action_condition::scope_to_side<true>(pd, bd, nd, nd->info_.data_);
	// current
	node_add_instr(pd, nd);
}
// ? |
void actions::do_leaf_then_else(prepare_data * pd, tree * tr, node * parent, node * nd) {
	// prepare
	body * bd = pd->body_.h_;
	// then part
	nd->instr_.params_.data_ = action_condition::scope_to_side<true>(pd, bd, nd, nd->info_.data_);
	// else part
	nd->instr_.params_.extra_ = action_condition::scope_to_side<true>(pd, bd, nd, nd->info_.extra_);
	// current
	node_add_instr(pd, nd);
}

void actions::turn_leaf_then_to_else(node * nd) {
	nd->instr_.type_ = mod::instructions::get_then_else();
	nd->info_.action_ = do_leaf_then_else;
}

void actions::do_leaf_while(prepare_data * pd, tree * tr, node * parent, node * nd) {
	// prepare
	body * bd = pd->body_.h_;
	// condition
	nd->instr_.params_.data_ = action_condition::scope_to_side<true>(pd, bd, nd, nd->info_.data_);
	// primary loop body
	nd->instr_.params_.extra_ = action_condition::scope_to_side<false>(pd, bd, nd, nd->info_.extra_);
	// rest loop body, maybe
	if( nd->info_.mod_ )
	nd->instr_.params_.mod_ = action_condition::scope_to_side<false>(pd, bd, nd, nd->info_.mod_);
	// additional block, maybe
	if( nd->info_.also_ )
	nd->instr_.params_.also_ = action_condition::scope_to_side<false>(pd, bd, nd, nd->info_.also_);
	// current
	node_add_instr(pd, nd);
}

void actions::prefix_operator::do_leaf_assoc_left(prepare_data * pd, tree * tr, node * parent, node * nd) {
	body * bd = pd->body_.h_;
	scope * sc = bd->scopes_.items_[nd->info_.data_];
	sc->i_first_.type_ = mod::instructions::get_nothing();
	sc->i_step_ = nd->instr_;
	sc->i_last_ = nd->instr_;
	sc->perform(pd);
}

void actions::prefix_operator::do_leaf_cmp_x_assoc_left(prepare_data * pd, tree * tr, node * parent, node * nd) {
	body * bd = pd->body_.h_;
	scope * sc = bd->scopes_.items_[nd->info_.data_];
	tree
		* first = *sc->trees_.items_,
		* last = sc->trees_.last(0)
	;
	node * root;
	id depth = sc->trees_.count_ -2;
	for( tree * it : sc->trees_ ) {
		root = *it->nodes_.items_;
		root->info_.action_(pd, it, root, root);
		if( it == last ) {
			node_add_instr(pd, nd);
			if( depth )
			bd->del_side_pos(depth);
		} else if( it != first ) {
			mod::side * sd = bd->current_side();
			mod::instr tmp = nd->instr_;
			tmp.type_ = mod::instructions::turn_cmp_x(tmp.type_);
			tmp.params_.extra_ = bd->add_side_pos();
			sd->add_instr(tmp);
		}
	}
}

void actions::prefix_operator::do_leaf_concat_assoc_left(prepare_data * pd, tree * tr, node * parent, node * nd) {
	body * bd = pd->body_.h_;
	scope * sc = bd->scopes_.items_[nd->info_.data_];
	id count = sc->trees_.count_;
	if( count > 2 ) {
		mod::side * sd = bd->current_side();
		sd->add_instr({
			mod::instructions::get_for_implode_make(),
			{nd->instr_.params_.pos_, count}
		});
		//
		{
			tree * first = *sc->trees_.items_;
			node * root;
			for( tree * it : sc->trees_ ) {
				root = *it->nodes_.items_;
				root->info_.action_(pd, it, root, root);
				also::t_pos pos = it->leafs_.count_ ? it->leafs_.items_[0]->instr_.params_.pos_ : nd->instr_.params_.pos_;
				sd->add_instr({
					mod::instructions::get_for_implode_add(),
					{pos, (it == first ? 1 : 2)}
				});
			}
		}
		sd->add_instr({
			mod::instructions::get_for_implode_go(),
			{nd->instr_.params_.pos_}
		});
	} else {
		node * root;
		for( tree * it : sc->trees_ ) {
			root = *it->nodes_.items_;
			root->info_.action_(pd, it, root, root);
		}
		node_add_instr(pd, nd);
	}
}

void actions::prefix_operator::do_leaf_concat_assoc_right(prepare_data * pd, tree * tr, node * parent, node * nd) {
	body * bd = pd->body_.h_;
	scope * sc = bd->scopes_.items_[nd->info_.data_];
	id count = sc->trees_.count_;
	if( count > 2 ) {
		mod::side * sd = bd->current_side();
		sd->add_instr({
			mod::instructions::get_for_implode_make(),
			{nd->instr_.params_.pos_, count}
		});
		//
		{
			tree * first = *sc->trees_.items_;
			node * root;
			for( tree * it : sc->trees_.get_rev_iter() ) {
				root = *it->nodes_.items_;
				root->info_.action_(pd, it, root, root);
				also::t_pos pos = it->leafs_.count_ ? it->leafs_.items_[0]->instr_.params_.pos_ : nd->instr_.params_.pos_;
				sd->add_instr({
					mod::instructions::get_for_implode_add(),
					{pos, (it == first ? 1 : 2)}
				});
			}
		}
		sd->add_instr({
			mod::instructions::get_for_implode_go(),
			{nd->instr_.params_.pos_}
		});
	} else {
		node * root;
		for( tree * it : sc->trees_.get_rev_iter() ) {
			root = *it->nodes_.items_;
			root->info_.action_(pd, it, root, root);
		}
		node_add_instr(pd, nd);
	}
}

void actions::prefix_operator::do_leaf_lazy_assoc_left(prepare_data * pd, tree * tr, node * parent, node * nd) {
	body * bd = pd->body_.h_;
	scope * sc = bd->scopes_.items_[nd->info_.data_];
	id depth = sc->trees_.count_ -1;
	tree * last = sc->trees_.last(0);
	node * root;
	for( tree * it : sc->trees_ ) {
		root = *it->nodes_.items_;
		root->info_.action_(pd, it, root, root);
		if( it != last ) {
			mod::side * sd = bd->current_side();
			nd->instr_.params_.extra_ = bd->add_side_pos();
			sd->add_instr(nd->instr_);
		}
	}
	bd->del_side_pos(depth);
}

template <bool Is_set>
struct action_dot {
	static inline void impl(prepare_data * pd, tree * tr, node * parent, node * nd) {
		if( !actions::is_node_dive(parent) ) {
			id depth = actions::calc_dive_depth(pd, nd);
			if( depth > 1 ) {
				mod::side * sd = pd->body_.h_->current_side();
				sd->add_instr({
					mod::instructions::get_array_put_exact(),
					{nd->instr_.params_.pos_, depth}
				});
				// right
				nd->rt_->info_.action_(pd, tr, nd, nd->rt_);
				if( nd->rt_->info_.kind_ == nk_bracket_args )
				sd->add_instr({
					mod::instructions::get_array_append(),
					{nd->rt_->instr_.params_.pos_}
				});
				// left
				nd->lt_->info_.action_(pd, tr, nd, nd->lt_);
				//
				if constexpr( Is_set ) {
					sd->add_instr({
						mod::instructions::get_link_dive_deep(),
						{nd->instr_.params_.pos_}
					});
				} else {
					sd->add_instr({
						mod::instructions::get_dive_deep(),
						{nd->instr_.params_.pos_}
					});
				}
			} else {
				actions::do_node_assoc_right(pd, tr, parent, nd);
			}
		} else {
			mod::side * sd = pd->body_.h_->current_side();
			// left
			nd->lt_->info_.action_(pd, tr, nd, nd->lt_);
			sd->add_instr({
				mod::instructions::get_array_append(),
				{nd->instr_.params_.pos_}
			});
			// right
			nd->rt_->info_.action_(pd, tr, nd, nd->rt_);
			if( !actions::is_node_dive(nd->rt_) ) {
				sd->add_instr({
					mod::instructions::get_array_append(),
					{nd->instr_.params_.pos_}
				});
			}
		}
	}
};

void actions::do_dot_get(prepare_data * pd, tree * tr, node * parent, node * nd) {
	action_dot<false>::impl(pd, tr, parent, nd);
}

void actions::do_dot_set(prepare_data * pd, tree * tr, node * parent, node * nd) {
	action_dot<true>::impl(pd, tr, parent, nd);
}

array_iter<op_info> actions::iter_op_assign() {
	static op_info ops[] = {
		{
			{L"="}, {
			{do_node_assoc_right, prec_x_assign, prec_assign},
			tree::add_node_assoc_right,
			mod::instructions::get_assign_value() }
		}, {
			{L"+="}, {
			{do_node_assoc_right, prec_x_assign, prec_assign},
			tree::add_node_assoc_right,
			mod::instructions::get_assign_value_with_op(), 0 }
		}, {
			{L"-="}, {
			{do_node_assoc_right, prec_x_assign, prec_assign},
			tree::add_node_assoc_right,
			mod::instructions::get_assign_value_with_op(), 1 }
		}, {
			{L"*="}, {
			{do_node_assoc_right, prec_x_assign, prec_assign},
			tree::add_node_assoc_right,
			mod::instructions::get_assign_value_with_op(), 2 }
		}, {
			{L"/="}, {
			{do_node_assoc_right, prec_x_assign, prec_assign},
			tree::add_node_assoc_right,
			mod::instructions::get_assign_value_with_op(), 3 }
		}, {
			{L"`mod="}, {
			{do_node_assoc_right, prec_x_assign, prec_assign},
			tree::add_node_assoc_right,
			mod::instructions::get_assign_value_with_op(), 4 }
		}, {
			{L"%="}, {
			{do_node_assoc_right, prec_x_assign, prec_assign},
			tree::add_node_assoc_right,
			mod::instructions::get_assign_value_with_op(), 5 }
		}, {
			{L"%%="}, {
			{do_node_assoc_right, prec_x_assign, prec_assign},
			tree::add_node_assoc_right,
			mod::instructions::get_assign_value_with_op(), 6 }
		}, {
			{L"`xor="}, {
			{do_node_assoc_right, prec_x_assign, prec_assign},
			tree::add_node_assoc_right,
			mod::instructions::get_assign_value_with_op(), 7 }
		}, {
			{L"`_and="}, {
			{do_node_assoc_right, prec_x_assign, prec_assign},
			tree::add_node_assoc_right,
			mod::instructions::get_assign_value_with_op(), 8 }
		}, {
			{L"`_or="}, {
			{do_node_assoc_right, prec_x_assign, prec_assign},
			tree::add_node_assoc_right,
			mod::instructions::get_assign_value_with_op(), 9 }
		}, {
			{L"`_xor="}, {
			{do_node_assoc_right, prec_x_assign, prec_assign},
			tree::add_node_assoc_right,
			mod::instructions::get_assign_value_with_op(), 10 }
		}, {
			{L"?\?="}, {
			{do_lazy_assoc_left, prec_x_assign, prec_assign},
			tree::add_node_assoc_right,
			mod::instructions::get_lazy_assign_nullc() }
		}, {
			{L"`and="}, {
			{do_lazy_assoc_left, prec_x_assign, prec_assign},
			tree::add_node_assoc_right,
			mod::instructions::get_lazy_assign_bool_and() }
		}, {
			{L"`or="}, {
			{do_lazy_assoc_left, prec_x_assign, prec_assign},
			tree::add_node_assoc_right,
			mod::instructions::get_lazy_assign_bool_or() }
		}
	};
	return ops;
}

array_iter<op_info> actions::iter_op_assign_rt() {
	static op_info ops[] = {
		{
			{L"=>"}, {
			{do_node_assoc_left, prec_xrt_assign, prec_rt_assign},
			tree::add_node_assoc_left,
			mod::instructions::get_assign_value() }
		}, {
			{L"+=>"}, {
			{do_node_assoc_left, prec_xrt_assign, prec_rt_assign},
			tree::add_node_assoc_left,
			mod::instructions::get_assign_value_with_op(), 0 }
		}, {
			{L"-=>"}, {
			{do_node_assoc_left, prec_xrt_assign, prec_rt_assign},
			tree::add_node_assoc_left,
			mod::instructions::get_assign_value_with_op(), 1 }
		}, {
			{L"*=>"}, {
			{do_node_assoc_left, prec_xrt_assign, prec_rt_assign},
			tree::add_node_assoc_left,
			mod::instructions::get_assign_value_with_op(), 2 }
		}, {
			{L"/=>"}, {
			{do_node_assoc_left, prec_xrt_assign, prec_rt_assign},
			tree::add_node_assoc_left,
			mod::instructions::get_assign_value_with_op(), 3 }
		}, {
			{L"`mod=>"}, {
			{do_node_assoc_left, prec_xrt_assign, prec_rt_assign},
			tree::add_node_assoc_left,
			mod::instructions::get_assign_value_with_op(), 4 }
		}, {
			{L"%=>"}, {
			{do_node_assoc_left, prec_xrt_assign, prec_rt_assign},
			tree::add_node_assoc_left,
			mod::instructions::get_assign_value_with_op(), 5 }
		}, {
			{L"%%=>"}, {
			{do_node_assoc_left, prec_xrt_assign, prec_rt_assign},
			tree::add_node_assoc_left,
			mod::instructions::get_assign_value_with_op(), 6 }
		}, {
			{L"`xor=>"}, {
			{do_node_assoc_left, prec_xrt_assign, prec_rt_assign},
			tree::add_node_assoc_left,
			mod::instructions::get_assign_value_with_op(), 7 }
		}, {
			{L"`_and=>"}, {
			{do_node_assoc_left, prec_xrt_assign, prec_rt_assign},
			tree::add_node_assoc_left,
			mod::instructions::get_assign_value_with_op(), 8 }
		}, {
			{L"`_or=>"}, {
			{do_node_assoc_left, prec_xrt_assign, prec_rt_assign},
			tree::add_node_assoc_left,
			mod::instructions::get_assign_value_with_op(), 9 }
		}, {
			{L"`_xor=>"}, {
			{do_node_assoc_left, prec_xrt_assign, prec_rt_assign},
			tree::add_node_assoc_left,
			mod::instructions::get_assign_value_with_op(), 10 }
		}, {
			{L"?\?=>"}, {
			{do_lazy_assoc_right, prec_xrt_assign, prec_rt_assign},
			tree::add_node_assoc_left,
			mod::instructions::get_lazy_assign_nullc() }
		}, {
			{L"`and=>"}, {
			{do_lazy_assoc_right, prec_xrt_assign, prec_rt_assign},
			tree::add_node_assoc_left,
			mod::instructions::get_lazy_assign_bool_and() }
		}, {
			{L"`or=>"}, {
			{do_lazy_assoc_right, prec_xrt_assign, prec_rt_assign},
			tree::add_node_assoc_left,
			mod::instructions::get_lazy_assign_bool_or() }
		}
	};
	return ops;
}

array_iter<op_info> actions::iter_op() {
	static op_info ops[] = {
		{
			{L"+"}, {
			{do_node_assoc_left, prec_plus, prec_plus},
			tree::add_node_assoc_left,
			mod::instructions::get_plus() }
		}, {
			{L"-"}, {
			{do_node_assoc_left, prec_plus, prec_plus},
			tree::add_node_assoc_left,
			mod::instructions::get_minus() }
		}, {
			{L"*"}, {
			{do_node_assoc_left, prec_mult, prec_mult},
			tree::add_node_assoc_left,
			mod::instructions::get_mult() }
		}, {
			{L"/"}, {
			{do_node_assoc_left, prec_mult, prec_mult},
			tree::add_node_assoc_left,
			mod::instructions::get_div() }
		}, {
			{L"<=>"}, {
			{do_node_assoc_left, prec_cmp, prec_cmp},
			tree::add_node_assoc_left,
			mod::instructions::get_cmp() }
		}, {
			{L"=="}, {
			{do_cmp_x_assoc_left<mod::instructions, nk_cmp>, prec_eq, prec_eq, nk_cmp},
			tree::add_node_assoc_left,
			mod::instructions::get_cmp_is(), ex::cmp::equal }
		}, {
			{L"<>"}, {
			{do_cmp_x_assoc_left<mod::instructions, nk_cmp>, prec_eq, prec_eq, nk_cmp},
			tree::add_node_assoc_left,
			mod::instructions::get_cmp_is_not(), ex::cmp::equal }
		}, {
			{L"<="}, {
			{do_cmp_x_assoc_left<mod::instructions, nk_cmp>, prec_eq, prec_eq, nk_cmp},
			tree::add_node_assoc_left,
			mod::instructions::get_cmp_is_not(), ex::cmp::more }
		}, {
			{L">="}, {
			{do_cmp_x_assoc_left<mod::instructions, nk_cmp>, prec_eq, prec_eq, nk_cmp},
			tree::add_node_assoc_left,
			mod::instructions::get_cmp_is_not(), ex::cmp::less }
		}, {
			{L"<"}, {
			{do_cmp_x_assoc_left<mod::instructions, nk_cmp>, prec_eq, prec_eq, nk_cmp},
			tree::add_node_assoc_left,
			mod::instructions::get_cmp_is(), ex::cmp::less }
		}, {
			{L">"}, {
			{do_cmp_x_assoc_left<mod::instructions, nk_cmp>, prec_eq, prec_eq, nk_cmp},
			tree::add_node_assoc_left,
			mod::instructions::get_cmp_is(), ex::cmp::more }
		}, {
			{L"%%"}, {
			{do_concat_assoc_right, prec_plus, prec_plus, nk_concat},
			tree::add_node_assoc_left,
			mod::instructions::get_concat() }
		}, {
			{L"%"}, {
			{do_concat_assoc_left, prec_plus, prec_plus, nk_concat},
			tree::add_node_assoc_left,
			mod::instructions::get_concat() }
		}, {
			{L"??"}, {
			{do_lazy_assoc_left, prec_nullc, prec_nullc},
			tree::add_node_assoc_right,
			mod::instructions::get_null_coalesce() }
		}
	};
	return ops;
}

array_iter<op_info> actions::iter_prefix_op() {
	static op_info ops[] = {
		{
			{L"+"}, {
			{prefix_operator::do_leaf_assoc_left, prec_leaf, prec_leaf},
			tree::add_leaf,
			mod::instructions::get_plus() }
		}, {
			{L"-"}, {
			{prefix_operator::do_leaf_assoc_left, prec_leaf, prec_leaf},
			tree::add_leaf,
			mod::instructions::get_minus() }
		}, {
			{L"*"}, {
			{prefix_operator::do_leaf_assoc_left, prec_leaf, prec_leaf},
			tree::add_leaf,
			mod::instructions::get_mult() }
		}, {
			{L"/"}, {
			{prefix_operator::do_leaf_assoc_left, prec_leaf, prec_leaf},
			tree::add_leaf,
			mod::instructions::get_div() }
		}, {
			{L"<=>"}, {
			{prefix_operator::do_leaf_assoc_left, prec_leaf, prec_leaf},
			tree::add_leaf,
			mod::instructions::get_cmp() }
		}, {
			{L"=="}, {
			{prefix_operator::do_leaf_cmp_x_assoc_left, prec_leaf, prec_leaf},
			tree::add_leaf,
			mod::instructions::get_cmp_is(), ex::cmp::equal }
		}, {
			{L"<>"}, {
			{prefix_operator::do_leaf_cmp_x_assoc_left, prec_leaf, prec_leaf},
			tree::add_leaf,
			mod::instructions::get_cmp_is_not(), ex::cmp::equal }
		}, {
			{L"<="}, {
			{prefix_operator::do_leaf_cmp_x_assoc_left, prec_leaf, prec_leaf},
			tree::add_leaf,
			mod::instructions::get_cmp_is_not(), ex::cmp::more }
		}, {
			{L">="}, {
			{prefix_operator::do_leaf_cmp_x_assoc_left, prec_leaf, prec_leaf},
			tree::add_leaf,
			mod::instructions::get_cmp_is_not(), ex::cmp::less }
		}, {
			{L"<"}, {
			{prefix_operator::do_leaf_cmp_x_assoc_left, prec_leaf, prec_leaf},
			tree::add_leaf,
			mod::instructions::get_cmp_is(), ex::cmp::less }
		}, {
			{L">"}, {
			{prefix_operator::do_leaf_cmp_x_assoc_left, prec_leaf, prec_leaf},
			tree::add_leaf,
			mod::instructions::get_cmp_is(), ex::cmp::more }
		}, {
			{L"%%"}, {
			{prefix_operator::do_leaf_concat_assoc_right, prec_leaf, prec_leaf},
			tree::add_leaf,
			mod::instructions::get_concat() }
		}, {
			{L"%"}, {
			{prefix_operator::do_leaf_concat_assoc_left, prec_leaf, prec_leaf},
			tree::add_leaf,
			mod::instructions::get_concat() }
		}, {
			{L"??"}, {
			{prefix_operator::do_leaf_lazy_assoc_left, prec_leaf, prec_leaf},
			tree::add_leaf,
			mod::instructions::get_null_coalesce() }
		}
	};
	return ops;
}

actions::t_map_named_ops & actions::map_named_ops() {
	static t_map_named_ops ops(1, ex::same_key::update, {
		{L"`mod", {
			{do_node_assoc_left, prec_mult, prec_mult},
			tree::add_node_assoc_left,
			mod::instructions::get_mod()
		}}, {L"`and", {
			{do_lazy_assoc_left, prec_and, prec_and},
			tree::add_node_assoc_right,
			mod::instructions::get_and()
		}}, {L"`or", {
			{do_lazy_assoc_left, prec_or, prec_or},
			tree::add_node_assoc_right,
			mod::instructions::get_or()
		}}, {L"`xor", {
			{do_node_assoc_left, prec_xor, prec_xor},
			tree::add_node_assoc_left,
			mod::instructions::get_xor()
		}}, {L"`_and", {
			{do_node_assoc_left, prec_and_, prec_and_},
			tree::add_node_assoc_left,
			mod::instructions::get_bit_and()
		}}, {L"`_or", {
			{do_node_assoc_left, prec_or_, prec_or_},
			tree::add_node_assoc_left,
			mod::instructions::get_bit_or()
		}}, {L"`_xor", {
			{do_node_assoc_left, prec_xor_, prec_xor_},
			tree::add_node_assoc_left,
			mod::instructions::get_bit_xor()
		}}
	});
	return ops;
}

actions::t_map_named_ops & actions::map_prefix_named_ops() {
	static t_map_named_ops ops(1, ex::same_key::update, {
		{L"`mod", {
			{prefix_operator::do_leaf_assoc_left, prec_leaf, prec_leaf},
			tree::add_leaf,
			mod::instructions::get_mod()
		}}, {L"`and", {
			{prefix_operator::do_leaf_lazy_assoc_left, prec_leaf, prec_leaf},
			tree::add_leaf,
			mod::instructions::get_and()
		}}, {L"`or", {
			{prefix_operator::do_leaf_lazy_assoc_left, prec_leaf, prec_leaf},
			tree::add_leaf,
			mod::instructions::get_or()
		}}, {L"`xor", {
			{prefix_operator::do_leaf_assoc_left, prec_leaf, prec_leaf},
			tree::add_leaf,
			mod::instructions::get_xor()
		}}, {L"`_and", {
			{prefix_operator::do_leaf_assoc_left, prec_leaf, prec_leaf},
			tree::add_leaf,
			mod::instructions::get_bit_and()
		}}, {L"`_or", {
			{prefix_operator::do_leaf_assoc_left, prec_leaf, prec_leaf},
			tree::add_leaf,
			mod::instructions::get_bit_or()
		}}, {L"`_xor", {
			{prefix_operator::do_leaf_assoc_left, prec_leaf, prec_leaf},
			tree::add_leaf,
			mod::instructions::get_bit_xor()
		}}
	});
	return ops;
}

// ?
op_info_lite * actions::op_then() {
	static op_info_lite op = {
		{do_none_assoc_left, prec_x_then, prec_then},
		tree::add_node_assoc_left,
		mod::instructions::get_nothing()
	};
	return &op;
}

// each
op_info_lite * actions::op_each() {
	static op_info_lite op = {
		{do_each_assoc_left, prec_x_then, prec_then},
		tree::add_node_assoc_left,
		mod::instructions::get_each()
	};
	return &op;
}

// for
op_info_lite * actions::op_for() {
	static op_info_lite op = {
		{do_each_assoc_left, prec_x_then, prec_then},
		tree::add_node_assoc_left,
		mod::instructions::get_loop_for()
	};
	return &op;
}

// :
op_info_lite * actions::op_colon() {
	static op_info_lite op = {
		{do_none_assoc_left, prec_pair, prec_pair, nk_pair},
		tree::add_node_assoc_left,
		mod::instructions::get_nothing()
	};
	return &op;
}

// .
op_info_lite * actions::op_dot_get() {
	static op_info_lite op = {
		{do_dot_get, prec_member, prec_member, nk_dot},
		tree::add_node_assoc_right,
		mod::instructions::get_dive()
	};
	return &op;
}

// .
op_info_lite * actions::op_dot_set() {
	static op_info_lite op = {
		{do_dot_set, prec_member, prec_member, nk_dot},
		tree::add_node_assoc_right,
		mod::instructions::get_link_dive()
	};
	return &op;
}

// []
op_info_lite * actions::op_bracket_get() {
	static op_info_lite op = {
		{do_dot_get, prec_member, prec_member, nk_bracket},
		tree::add_node_assoc_right,
		mod::instructions::get_dive()
	};
	return &op;
}

// []
op_info_lite * actions::op_bracket_set() {
	static op_info_lite op = {
		{do_dot_set, prec_member, prec_member, nk_bracket},
		tree::add_node_assoc_right,
		mod::instructions::get_link_dive()
	};
	return &op;
}

// a$
op_info_lite * actions::op_type_of() {
	static op_info_lite op = {
		{do_none_assoc_left, prec_x_then, prec_type_of},
		tree::add_node_assoc_left,
		mod::instructions::get_nothing()
	};
	return &op;
}

} // ns
} // ns
