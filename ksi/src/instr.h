#pragma once
#include "space.h"

namespace ksi {
namespace mod {

struct event_break {
	id depth_;
};

struct event_next {
	id depth_;
};

#define FN_GET_INSTR_TYPE(v_ins) \
	static const instr_type * get_ ## v_ins() { \
		static const instr_type ret = {L ## #v_ins, do_ ## v_ins}; \
		return &ret; \
	}
#define FN_GET_INSTR_TYPE_2(v_ins, v_func) \
	static const instr_type * get_ ## v_ins() { \
		static const instr_type ret = {L ## #v_ins, v_func}; \
		return &ret; \
	}

struct instructions {
	static void do_nothing(space * spc, fn_space * fns, t_stack * stk, base_log * log, const instr_data & params) {}
	static const instr_type * get_nothing() {
		static const instr_type ret = {L"nothing", do_nothing, true};
		return &ret;
	}

	// lit_null: puts null to stack
	static void do_lit_null(space * spc, fn_space * fns, t_stack * stk, base_log * log, const instr_data & params) {
		stk->items_.append_obj(var::n_null::val);
	}
	FN_GET_INSTR_TYPE(lit_null)

	// lit_bool: puts bool value to stack (params.data_ ~ true or false)
	static void do_lit_bool(space * spc, fn_space * fns, t_stack * stk, base_log * log, const instr_data & params) {
		stk->items_.append_obj(static_cast<bool>(params.data_) );
	}
	FN_GET_INSTR_TYPE(lit_bool)

	// lit_int: puts int value to stack (params.data_ ~ value)
	static void do_lit_int(space * spc, fn_space * fns, t_stack * stk, base_log * log, const instr_data & params) {
		stk->items_.append_obj(params.data_);
	}
	FN_GET_INSTR_TYPE(lit_int)

	// literal: puts constant from module to stack (params.data_ ~ pos of lit)
	static void do_literal(space * spc, fn_space * fns, t_stack * stk, base_log * log, const instr_data & params) {
		stk->items_.append(fns->mod_->lit_.items_[params.data_]);
	}
	FN_GET_INSTR_TYPE(literal)

	// type: puts type from space to stack (params.data_ ~ pos of type)
	static void do_type(space * spc, fn_space * fns, t_stack * stk, base_log * log, const instr_data & params) {
		stk->items_.append(spc->types_.arr_.items_[params.data_]);
	}
	FN_GET_INSTR_TYPE(type)

	// var_get: puts value of variable to stack (params.data_ ~ pos of var)
	static void do_var_get(space * spc, fn_space * fns, t_stack * stk, base_log * log, const instr_data & params) {
		stk->items_.append(fns->vars_[params.data_].h_->val_);
	}
	FN_GET_INSTR_TYPE(var_get)

	// var_target: puts link of variable to stack (params.data_ ~ pos of var)
	static void do_var_target(space * spc, fn_space * fns, t_stack * stk, base_log * log, const instr_data & params) {
		stk->items_.append_obj(fns->vars_ + params.data_);
	}
	FN_GET_INSTR_TYPE(var_target)

	// assign_value: assign value to link and removes link from stack
	static void do_assign_value(space * spc, fn_space * fns, t_stack * stk, base_log * log, const instr_data & params) {
		var::any * target = &stk->items_.last(0);
		if( var::keep_array * ka = var::keep_array::any_link_check(*target) ) {
			var::ref_var * link = ka->any_link_get();
			*link = stk->items_.last(1);
		}
		stk->items_.remove_last_n(1);
	}
	FN_GET_INSTR_TYPE(assign_value)

	// assign_value_with_op: (params.data_ ~ position of op)
	static void do_assign_value_with_op(space * spc, fn_space * fns, t_stack * stk, base_log * log, const instr_data & params) {
		var::any * target = &stk->items_.last(0);
		if( var::keep_array * ka = var::keep_array::any_link_check(*target) ) {
			var::ref_var * link = ka->any_link_get();
			stk->items_.append(link->h_->val_);
			stk->items_.append(stk->items_.last(2) );
			static const instr_type * ops[] = {
				get_plus(),			// 0: +
				get_minus(),		// 1: -
				get_mult(),			// 2: *
				get_div(),			// 3: /
				get_mod(),			// 4: `mod
				get_concat(),		// 5: %
				get_concat_back(),	// 6: %%
				get_xor(),			// 7: `xor
				get_bit_and(),		// 8: `_and
				get_bit_or(),		// 9: `_or
				get_bit_xor()		//10: `_xor
			};
			ops[params.data_]->perform_(spc, fns, stk, log, {params.pos_});
			var::any * result = &stk->items_.last(0);
			*link = *result;
			stk->items_.last(2) = *result;
			stk->items_.remove_last_n(2);
		} else
		stk->items_.remove_last_n(1);
	}
	FN_GET_INSTR_TYPE(assign_value_with_op)

	// array_put: (params.data_ ~ array reserve)
	static void do_array_put(space * spc, fn_space * fns, t_stack * stk, base_log * log, const instr_data & params) {
		stk->items_.append_obj( new var::keep_array(params.data_) );
	}
	FN_GET_INSTR_TYPE(array_put)

	// array_put_exact: (params.data_ ~ array reserve)
	static void do_array_put_exact(space * spc, fn_space * fns, t_stack * stk, base_log * log, const instr_data & params) {
		stk->items_.append_obj( new var::keep_array(params.data_, 0) );
	}
	FN_GET_INSTR_TYPE(array_put_exact)

	// array_append
	static void do_array_append(space * spc, fn_space * fns, t_stack * stk, base_log * log, const instr_data & params) {
		var::any * arr = &stk->items_.last(1);
		if( arr->type_ == &var::hcfg->t_array ) {
			var::keep_array * ka = arr->value_.keep_->k_array();
			ka->items_.append( stk->items_.last(0) );
		}
		stk->items_.remove_last_n(1);
	}
	FN_GET_INSTR_TYPE(array_append)

	// map_put: (params.data_ ~ map reserve)
	static void do_map_put(space * spc, fn_space * fns, t_stack * stk, base_log * log, const instr_data & params) {
		stk->items_.append_obj( new var::keep_map(params.data_) );
	}
	FN_GET_INSTR_TYPE(map_put)

	// map_append_1
	static void do_map_append_1(space * spc, fn_space * fns, t_stack * stk, base_log * log, const instr_data & params) {
		var::any * arr = &stk->items_.last(1);
		if( arr->type_ == &var::hcfg->t_map ) {
			var::keep_map * km = arr->value_.keep_->k_map();
			wtext msg;
			km->set(var::n_null::val, stk->items_.last(0), msg, true);
			if( msg )
			log->add({msg, fns->mod_->path_, params.pos_});
		}
		stk->items_.remove_last_n(1);
	}
	FN_GET_INSTR_TYPE(map_append_1)

	// map_append_2
	static void do_map_append_2(space * spc, fn_space * fns, t_stack * stk, base_log * log, const instr_data & params) {
		var::any * arr = &stk->items_.last(2);
		if( arr->type_ == &var::hcfg->t_map ) {
			var::keep_map * km = arr->value_.keep_->k_map();
			wtext msg;
			km->set(stk->items_.last(1), stk->items_.last(0), msg, true);
			if( msg )
			log->add({msg, fns->mod_->path_, params.pos_});
		}
		stk->items_.remove_last_n(2);
	}
	FN_GET_INSTR_TYPE(map_append_2)

	// dive: operator dot get
	static void do_dive(space * spc, fn_space * fns, t_stack * stk, base_log * log, const instr_data & params) {
		var::any * target = &stk->items_.last(0);
		*target = target->type_->element_get(*target, stk->items_.last(1) );
		stk->items_.remove_from_end(1);
	}
	FN_GET_INSTR_TYPE(dive)

	// dive_deep: operator dot get (array keys)
	static void do_dive_deep(space * spc, fn_space * fns, t_stack * stk, base_log * log, const instr_data & params) {
		var::any * target = &stk->items_.last(0), * keys = &stk->items_.last(1);
		if( keys->type_ == &var::hcfg->t_array && target->type_ != &var::hcfg->t_null )
		for( var::ref_var & key : keys->value_.keep_->k_array()->items_ ) {
			*target = target->type_->element_get(*target, key.h_->val_);
			if( target->type_ == &var::hcfg->t_null )
			break;
		}
		stk->items_.remove_from_end(1);
	}
	FN_GET_INSTR_TYPE(dive_deep)

	// link_dive: operator dot set
	static void do_link_dive(space * spc, fn_space * fns, t_stack * stk, base_log * log, const instr_data & params) {
		var::any * target = &stk->items_.last(0);
		if( var::keep_array * ka = var::keep_array::any_link_check(*target) ) {
			wtext msg;
			var::ref_var * link = ka->any_link_get();
			link->h_->val_.type_->element_set(*target, stk->items_.last(1), ka, msg);
			if( msg )
			log->add({msg, fns->mod_->path_, params.pos_});
		}
		stk->items_.remove_from_end(1);
	}
	FN_GET_INSTR_TYPE(link_dive)

	// link_dive_deep: operator dot set (array keys)
	static void do_link_dive_deep(space * spc, fn_space * fns, t_stack * stk, base_log * log, const instr_data & params) {
		var::any * target = &stk->items_.last(0), * keys = &stk->items_.last(1);
		if(
			var::keep_array * ka = var::keep_array::any_link_check(*target);
			ka && keys->type_ == &var::hcfg->t_array
		) for( var::ref_var & key : keys->value_.keep_->k_array()->items_ ) {
			wtext msg;
			var::ref_var * link = ka->any_link_get();
			bool good = link->h_->val_.type_->element_set(*target, key.h_->val_, ka, msg);
			if( msg )
			log->add({msg, fns->mod_->path_, params.pos_});

			if( !good )
			break;
		}
		stk->items_.remove_from_end(1);
	}
	FN_GET_INSTR_TYPE(link_dive_deep)

	// next_expr: remove last value from stack
	static void do_next_expr(space * spc, fn_space * fns, t_stack * stk, base_log * log, const instr_data & params) {
		stk->items_.remove_last_n(1);
	}
	FN_GET_INSTR_TYPE(next_expr)

	// fn_native_call_2: (params.data_ ~ pos in fn_map_)
	static void do_fn_native_call_2(space * spc, fn_space * fns, t_stack * stk, base_log * log, const instr_data & params) {
		var::hcfg->native_->fn_map_.arr_.items_[params.data_]->call_2(spc, fns, stk, params.pos_, log);
	}
	FN_GET_INSTR_TYPE(fn_native_call_2)

	// fn_native_call_2_bk: (params.data_ ~ pos in fn_map_)
	static void do_fn_native_call_2_bk(space * spc, fn_space * fns, t_stack * stk, base_log * log, const instr_data & params) {
		var::hcfg->native_->fn_map_.arr_.items_[params.data_]->call_2_bk(spc, fns, stk, params.pos_, log);
	}
	FN_GET_INSTR_TYPE(fn_native_call_2_bk)

	// fn_global_call_2: (params.data_ ~ pos in fn_map_)
	static void do_fn_global_call_2(space * spc, fn_space * fns, t_stack * stk, base_log * log, const instr_data & params) {
		spc->fn_map_.arr_.items_[params.data_]->call_2(spc, fns, stk, params.pos_, log);
	}
	FN_GET_INSTR_TYPE(fn_global_call_2)

	// fn_global_call_2_bk: (params.data_ ~ pos in fn_map_)
	static void do_fn_global_call_2_bk(space * spc, fn_space * fns, t_stack * stk, base_log * log, const instr_data & params) {
		spc->fn_map_.arr_.items_[params.data_]->call_2_bk(spc, fns, stk, params.pos_, log);
	}
	FN_GET_INSTR_TYPE(fn_global_call_2_bk)

	template <class Op>
	static void inner_number_op(
		space * spc, fn_space * fns, t_stack * stk, base_log * log, const instr_data & params /*, const wtext & op_name */
	) {
		wtext msg1, msg2;
		stk->items_.last(1) = var::number_op<Op>::calc(stk->items_.last(1), stk->items_.last(0), msg1, msg2);
		stk->items_.remove_last_n(1);
		if( msg1 )
		log->add({ex::implode(
			{msg1, L" When doing math action ", Op::get_name(), L" (left operand)."}), fns->mod_->path_, params.pos_}
		);
		if( msg2 )
		log->add({ex::implode(
			{msg2, L" When doing math action ", Op::get_name(), L" (right operand)."}), fns->mod_->path_, params.pos_}
		);
	}

	// plus
	/* static void do_plus(space * spc, fn_space * fns, t_stack * stk, base_log * log, const instr_data & params) {
		inner_number_op<var::op_plus>(spc, fns, stk, log, params, L"addition");
	} */
	FN_GET_INSTR_TYPE_2(plus, inner_number_op<var::op_plus>)

	// minus
	/* static void do_minus(space * spc, fn_space * fns, t_stack * stk, base_log * log, const instr_data & params) {
		inner_number_op<var::op_minus>(spc, fns, stk, log, params, L"subtraction");
	} */
	FN_GET_INSTR_TYPE_2(minus, inner_number_op<var::op_minus>)

	// mult
	/* static void do_mult(space * spc, fn_space * fns, t_stack * stk, base_log * log, const instr_data & params) {
		inner_number_op<var::op_mult>(spc, fns, stk, log, params, L"multiplication");
	} */
	FN_GET_INSTR_TYPE_2(mult, inner_number_op<var::op_mult>)

	// div
	/* static void do_div(space * spc, fn_space * fns, t_stack * stk, base_log * log, const instr_data & params) {
		inner_number_op<var::op_div>(spc, fns, stk, log, params, L"division");
	} */
	FN_GET_INSTR_TYPE_2(div, inner_number_op<var::op_div>)

	// mod
	/* static void do_mod(space * spc, fn_space * fns, t_stack * stk, base_log * log, const instr_data & params) {
		inner_number_op<var::op_mod>(spc, fns, stk, log, params, L"modulo");
	} */
	FN_GET_INSTR_TYPE_2(mod, inner_number_op<var::op_mod>)

	struct op_bit_and	{ static id calc(id n1, id n2) { return n1 & n2; } static wtext get_name() { return L"`_and"; } };
	struct op_bit_or	{ static id calc(id n1, id n2) { return n1 | n2; } static wtext get_name() { return L"`_or" ; } };
	struct op_bit_xor	{ static id calc(id n1, id n2) { return n1 ^ n2; } static wtext get_name() { return L"`_xor"; } };

	template <class Op>
	static void inner_bit_op(
		space * spc, fn_space * fns, t_stack * stk, base_log * log, const instr_data & params /*, const wtext & op_name */
	) {
		wtext msg1, msg2;
		var::any
		* a1 = &stk->items_.last(1),
		* a2 = &stk->items_.last(0);
		id
		n1 = a1->type_->to_int(*a1, msg1),
		n2 = a2->type_->to_int(*a2, msg2);
		if( msg1 )
		log->add({ex::implode(
			{msg1, L" When doing bitwise operator ", Op::get_name(), L" (left operand)."}), fns->mod_->path_, params.pos_}
		);
		if( msg2 )
		log->add({ex::implode(
			{msg2, L" When doing bitwise operator ", Op::get_name(), L" (right operand)."}), fns->mod_->path_, params.pos_}
		);
		stk->items_.last(1) = Op::calc(n1, n2);
		stk->items_.remove_last_n(1);
	}

	// bit_and
	FN_GET_INSTR_TYPE_2(bit_and, inner_bit_op<op_bit_and>)

	// bit_or
	FN_GET_INSTR_TYPE_2(bit_or, inner_bit_op<op_bit_or>)

	// bit_xor
	FN_GET_INSTR_TYPE_2(bit_xor, inner_bit_op<op_bit_xor>)

	// concat
	template <bool IsBack>
	static void do_concat(space * spc, fn_space * fns, t_stack * stk, base_log * log, const instr_data & params) {
		var::any * h1 = &stk->items_.last(1), * h2 = &stk->items_.last(0);
		wtext msg1, msg2;
		wtext t1 = var::type_text::get_text(h1->type_->to_text(*h1, msg1) );
		wtext t2 = var::type_text::get_text(h2->type_->to_text(*h2, msg2) );
		if constexpr( IsBack )
		*h1 = ex::implode({t2, t1});
		else
		*h1 = ex::implode({t1, t2});

		stk->items_.remove_last_n(1);
		if( msg1 )
		log->add({ex::implode({msg1, L" When doing text concatenation (left operand)."}), fns->mod_->path_, params.pos_});
		if( msg2 )
		log->add({ex::implode({msg2, L" When doing text concatenation (right operand)."}), fns->mod_->path_, params.pos_});
	}
	static const instr_type * get_concat() {
		static const instr_type ret = {L"concat", do_concat<false>};
		return &ret;
	}
	static const instr_type * get_concat_back() {
		static const instr_type ret = {L"concat_back", do_concat<true>};
		return &ret;
	}

	// for_implode_make: (params.data_ ~ array reserve)
	static void do_for_implode_make(space * spc, fn_space * fns, t_stack * stk, base_log * log, const instr_data & params) {
		stk->for_implode_.append(new t_stack::for_implode(params.data_, 1) );
	}
	FN_GET_INSTR_TYPE(for_implode_make)

	// for_implode_add: (params.data_ ~ left:0 or right:1 operand)
	static void do_for_implode_add(space * spc, fn_space * fns, t_stack * stk, base_log * log, const instr_data & params) {
		var::any * h = &stk->items_.last(0);
		wtext msg, tx = var::type_text::get_text(h->type_->to_text(*h, msg) );
		stk->items_.remove_last_n(1);
		t_stack::for_implode * fi = stk->for_implode_.last(0);
		fi->append(tx);
		if( msg ) {
			wtext add_msg = L" When doing text concatenation (left operand).";
			if( params.data_ )
			add_msg = L" When doing text concatenation (right operand).";
			log->add({ex::implode({msg, add_msg}), fns->mod_->path_, params.pos_});
		}
	}
	FN_GET_INSTR_TYPE(for_implode_add)

	// for_implode_go
	static void do_for_implode_go(space * spc, fn_space * fns, t_stack * stk, base_log * log, const instr_data & params) {
		t_stack::for_implode * fi = stk->for_implode_.last(0);
		stk->items_.append_obj<const wtext &>( ex::implode(*fi) );
		stk->for_implode_.remove_last_n(1);
	}
	FN_GET_INSTR_TYPE(for_implode_go)

	// cmp
	static void do_cmp(space * spc, fn_space * fns, t_stack * stk, base_log * log, const instr_data & params) {
		var::any * v1 = &stk->items_.last(1);
		*v1 = v1->type_->compare(*v1, stk->items_.last(0) );
		stk->items_.remove_last_n(1);
	}
	FN_GET_INSTR_TYPE(cmp)

	// cmp_is: (params.data_ ~ is compare result)
	static void do_cmp_is(space * spc, fn_space * fns, t_stack * stk, base_log * log, const instr_data & params) {
		var::any * v1 = &stk->items_.last(1);
		*v1 = v1->type_->compare(*v1, stk->items_.last(0) ) == params.data_;
		stk->items_.remove_last_n(1);
	}
	FN_GET_INSTR_TYPE(cmp_is)

	// cmp_is_not: (params.data_ ~ is not compare result)
	static void do_cmp_is_not(space * spc, fn_space * fns, t_stack * stk, base_log * log, const instr_data & params) {
		var::any * v1 = &stk->items_.last(1);
		*v1 = v1->type_->compare(*v1, stk->items_.last(0) ) != params.data_;
		stk->items_.remove_last_n(1);
	}
	FN_GET_INSTR_TYPE(cmp_is_not)

	// cmp_x_is: (params.data_ ~ is compare result, params.extra_ ~ pos of side)
	static void do_cmp_x_is(space * spc, fn_space * fns, t_stack * stk, base_log * log, const instr_data & params) {
		var::any * v1 = &stk->items_.last(1);
		if( bool res = v1->type_->compare(*v1, stk->items_.last(0) ) == params.data_ ) {
			stk->items_.remove(stk->items_.count_ -2);
			fns->fn_body_->sides_.items_[params.extra_]->call(spc, fns, stk, log);
		} else {
			*v1 = res;
			stk->items_.remove_last_n(1);
		}
	}
	FN_GET_INSTR_TYPE(cmp_x_is)

	// cmp_x_is_not: (params.data_ ~ is not compare result, params.extra_ ~ pos of side)
	static void do_cmp_x_is_not(space * spc, fn_space * fns, t_stack * stk, base_log * log, const instr_data & params) {
		var::any * v1 = &stk->items_.last(1);
		if( bool res = v1->type_->compare(*v1, stk->items_.last(0) ) != params.data_ ) {
			stk->items_.remove(stk->items_.count_ -2);
			fns->fn_body_->sides_.items_[params.extra_]->call(spc, fns, stk, log);
		} else {
			*v1 = res;
			stk->items_.remove_last_n(1);
		}
	}
	FN_GET_INSTR_TYPE(cmp_x_is_not)

	static const instr_type * turn_cmp_x(const instr_type * itype) {
		return itype->perform_ == do_cmp_is ? get_cmp_x_is() : get_cmp_x_is_not();
	}

	// null_coalesce: (params.extra_ ~ pos of side)
	static void do_null_coalesce(space * spc, fn_space * fns, t_stack * stk, base_log * log, const instr_data & params) {
		if( stk->items_.last(0).type_ == &var::hcfg->t_null ) {
			stk->items_.remove_last_n(1);
			fns->fn_body_->sides_.items_[params.extra_]->call(spc, fns, stk, log);
		}
	}
	FN_GET_INSTR_TYPE(null_coalesce)

	// and: (params.extra_ ~ pos of side)
	static void do_and(space * spc, fn_space * fns, t_stack * stk, base_log * log, const instr_data & params) {
		var::any * a = &stk->items_.last(0);
		wtext msg;
		bool b = a->type_->to_bool(*a, msg);
		if( msg )
		log->add({ex::implode({msg, L" When doing logical operation `and (left operand)."}), fns->mod_->path_, params.pos_});
		if( b ) {
			stk->items_.remove_last_n(1);
			fns->fn_body_->sides_.items_[params.extra_]->call(spc, fns, stk, log);
		}
	}
	FN_GET_INSTR_TYPE(and)

	// or: (params.extra_ ~ pos of side)
	static void do_or(space * spc, fn_space * fns, t_stack * stk, base_log * log, const instr_data & params) {
		var::any * a = &stk->items_.last(0);
		wtext msg;
		bool b = a->type_->to_bool(*a, msg);
		if( msg )
		log->add({ex::implode({msg, L" When doing logical operation `or (left operand)."}), fns->mod_->path_, params.pos_});
		if( !b ) {
			stk->items_.remove_last_n(1);
			fns->fn_body_->sides_.items_[params.extra_]->call(spc, fns, stk, log);
		}
	}
	FN_GET_INSTR_TYPE(or)

	// xor
	static void do_xor(space * spc, fn_space * fns, t_stack * stk, base_log * log, const instr_data & params) {
		var::any * a1 = &stk->items_.last(1), * a2 = &stk->items_.last(0);
		wtext msg1, msg2;
		bool b1 = a1->type_->to_bool(*a1, msg1), b2 = a2->type_->to_bool(*a2, msg2);
		if( msg1 )
		log->add({ex::implode({msg1, L" When doing logical operation `xor (left operand)."}), fns->mod_->path_, params.pos_});
		if( msg2 )
		log->add({ex::implode({msg2, L" When doing logical operation `xor (right operand)."}), fns->mod_->path_, params.pos_});
		if( b1 != b2 ) {
			if( b1 )
			stk->items_.remove_last_n(1);
			else
			stk->items_.remove(stk->items_.count_ -2);
		} else {
			stk->items_.remove_last_n(1);
			stk->items_.last(0) = false;
		}
	}
	FN_GET_INSTR_TYPE(xor)

	// then: (params.data_ ~ pos of side true part)
	static void do_then(space * spc, fn_space * fns, t_stack * stk, base_log * log, const instr_data & params) {
		var::any * a = &stk->items_.last(0);
		wtext msg;
		bool b = a->type_->to_bool(*a, msg);
		if( msg )
		log->add({
			ex::implode({msg, L" In left operand of conditional expression (question mark)."}),
			fns->mod_->path_, params.pos_
		});
		if( b ) {
			stk->items_.remove_last_n(1);
			fns->fn_body_->sides_.items_[params.data_]->call(spc, fns, stk, log);
		} else {
			stk->items_.last(0) = var::n_null::val;
		}
	}
	FN_GET_INSTR_TYPE(then)

	// then_else: (params.data_ ~ pos of side true part, params.extra_ ~ pos of side false part)
	static void do_then_else(space * spc, fn_space * fns, t_stack * stk, base_log * log, const instr_data & params) {
		var::any * a = &stk->items_.last(0);
		wtext msg;
		bool b = a->type_->to_bool(*a, msg);
		if( msg )
		log->add({
			ex::implode({msg, L" In left operand of conditional expression (question mark)."}),
			fns->mod_->path_, params.pos_
		});
		stk->items_.remove_last_n(1);
		if( b )
		fns->fn_body_->sides_.items_[params.data_]->call(spc, fns, stk, log);
		else
		fns->fn_body_->sides_.items_[params.extra_]->call(spc, fns, stk, log);
	}
	FN_GET_INSTR_TYPE(then_else)

	template <bool Is_pre>
	inline static bool inner_while_condition(
		space * spc, fn_space * fns, t_stack * stk, base_log * log, const instr_data & params
	) {
		fns->fn_body_->sides_.items_[params.data_]->call(spc, fns, stk, log);
		var::any * a = &stk->items_.last(0);
		wtext msg;
		bool ret = a->type_->to_bool(*a, msg);
		if( msg ) {
			if constexpr( Is_pre )
			log->add({
				ex::implode({msg, L" In the pre-condition of loop while."}),
				fns->mod_->path_, params.pos_
			});
			else
			log->add({
				ex::implode({msg, L" In the post-condition of loop while."}),
				fns->mod_->path_, params.pos_
			});
		}
		stk->items_.remove_last_n(1);
		return ret;
	}

	inline static void inner_loop_block(
		space * spc, fn_space * fns, t_stack * stk, base_log * log,
		id side_pos, const t_stack::t_state & st
	) {
		try {
			fns->fn_body_->sides_.items_[side_pos]->call(spc, fns, stk, log);
		} catch( const event_next & e ) {
			if( e.depth_ == 1 )
			stk->state_restore(st);
			else
			throw event_next{e.depth_ -1};
		}
	}

	/* while_pre: (
		params.data_ ~ side pos of condition
		params.extra_ ~ side pos of loop body
		params.mod_ ~ side pos of loop body after '\' if any (0 if absent)
		params.also_ ~ side pos of additional block after condition and '\' before '~' if any (0 if absent)
	) */
	static void do_while_pre(space * spc, fn_space * fns, t_stack * stk, base_log * log, const instr_data & params) {
		t_stack::t_state st = stk->state_get();
		id steps = 0;
		try {
			bool b;
			id
				side_pos = params.extra_,
				rest_side_pos = (params.mod_ ? params.mod_ : side_pos)
			;
			if( params.also_ )
			do {
				// condition
				b = inner_while_condition<true>(spc, fns, stk, log, params);
				if( !b ) break;
				// loop body
				++steps;
				inner_loop_block(spc, fns, stk, log, side_pos, st); // primary side
				side_pos = rest_side_pos;
				inner_loop_block(spc, fns, stk, log, params.also_, st); // additional side
			} while( b );
			else
			do {
				// condition
				b = inner_while_condition<true>(spc, fns, stk, log, params);
				if( !b ) break;
				// loop body
				++steps;
				inner_loop_block(spc, fns, stk, log, side_pos, st); // primary side
				side_pos = rest_side_pos;
			} while( b );
		} catch( const event_break & e ) {
			if( e.depth_ == 1 )
			stk->state_restore(st);
			else
			throw event_break{e.depth_ -1};
		}
		stk->items_.append_obj(steps);
	}
	FN_GET_INSTR_TYPE(while_pre)

	/* while_post: (
		params.data_ ~ side pos of condition
		params.extra_ ~ side pos of loop body
		params.mod_ ~ side pos of loop body after '\' if any (0 if absent)
		params.also_ ~ side pos of additional block after condition and '\' before '~' if any (0 if absent)
	) */
	static void do_while_post(space * spc, fn_space * fns, t_stack * stk, base_log * log, const instr_data & params) {
		t_stack::t_state st = stk->state_get();
		id steps = 0;
		try {
			bool b;
			id
				side_pos = params.extra_,
				rest_side_pos = (params.mod_ ? params.mod_ : side_pos)
			;
			if( params.also_ )
			do {
				// loop body
				++steps;
				inner_loop_block(spc, fns, stk, log, side_pos, st); // primary side
				side_pos = rest_side_pos;
				inner_loop_block(spc, fns, stk, log, params.also_, st); // additional side
				// condition
				b = inner_while_condition<false>(spc, fns, stk, log, params);
			} while( b );
			else
			do {
				// loop body
				++steps;
				inner_loop_block(spc, fns, stk, log, side_pos, st); // primary side
				side_pos = rest_side_pos;
				// condition
				b = inner_while_condition<false>(spc, fns, stk, log, params);
			} while( b );
		} catch( const event_break & e ) {
			if( e.depth_ == 1 )
			stk->state_restore(st);
			else
			throw event_break{e.depth_ -1};
		}
		stk->items_.append_obj(steps);
	}
	FN_GET_INSTR_TYPE(while_post)

	// kw_break: (params.data_ ~ loop depth)
	static void do_kw_break(space * spc, fn_space * fns, t_stack * stk, base_log * log, const instr_data & params) {
		throw event_break{params.data_};
	}
	FN_GET_INSTR_TYPE(kw_break)

	// kw_next: (params.data_ ~ loop depth)
	static void do_kw_next(space * spc, fn_space * fns, t_stack * stk, base_log * log, const instr_data & params) {
		throw event_next{params.data_};
	}
	FN_GET_INSTR_TYPE(kw_next)
};

} // ns
} // ns
