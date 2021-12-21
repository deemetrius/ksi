#pragma once
#include "space.h"

namespace ksi {
namespace ast {

// precedence
enum n_prec {
	prec_leaf,		//	(ast leaf)
	prec_x_then,	//	_right_operand_: ? `each `for a$
	prec_member,	//	a.key a[key]
	prec_type_of,	//	a$
	prec_xrt_assign,//	_right_operand_: => +=> -=> *=> /=> %=> %%=> `and=> `or=> `xor=> ??=> `_and=> `_or=> `_xor=>
	prec_assign,	//	= += -= *= /= %= %%= `and= `or= `xor= ??= `_and= `_or= `_xor=
	prec_invoke,	//	! &				(fn call, fn ref)
	prec_mult,		//	* / `mod
	prec_and_,		//	`_and			(bitwise)
	prec_xor_,		//	`_xor
	prec_or_,		//	`_or
	prec_plus,		//	+ - % %% &user_fn #native_fn
	prec_cmp,		//	<=>
	prec_eq,		//	== <> < <= > >=
	prec_throw,		//	`throw
	prec_and,		//	`and			(logical)
	prec_xor,		//	`xor
	prec_or,		//	`or
	prec_nullc,		//	?? ?!			(null coalescing)
	prec_x_assign,	//	_right_operand_: = += -= *= /= %= %%= `and= `or= `xor= ??= `_and= `_or= `_xor=
	prec_rt_assign,	//	=> +=> -=> *=> /=> %=> %%=> `and=> `or=> `xor=> ??=> `_and=> `_or=> `_xor=>
	prec_then,		//	? `each `for
	prec_pair,		//	:				(key-value pair)
	prec_root,		//	(ast top)
};

// a + b = c	// a + (b = c)
// a => b += c	// not implemented atm
// a = b => c	// (a = b) => c

struct prepare_data;
struct tree;
struct node;

using hfn_action = void (*)(prepare_data * pd, tree * tr, node * parent, node * nd);

enum n_node_kind { nk_not_set, nk_concat, nk_pair, nk_dot, nk_bracket, nk_bracket_args, nk_cmp };

struct node_info {
	hfn_action action_;
	n_prec prec_, prec_search_;
	n_node_kind kind_ = nk_not_set;
	id data_ = 0, extra_ = 0, mod_ = 0, also_ = 0;
};

struct node {
	node_info info_;
	mod::instr instr_;
	node * lt_ = nullptr, * rt_ = nullptr;
};

//

struct tree;
using hfn_add_node = void (*)(tree *, node *);

struct op_info_str {
	wtext str_;
};
struct op_info_lite {
	node_info info_;
	hfn_add_node fn_adder_;
	const mod::instr_type * type_;
	id data_ = 0;
};
struct op_info : public op_info_str, public op_info_lite {};

//

struct tree {
	using t_items = ex::def_array<node *, ex::del_pointer, def::ast_nodes_r, def::ast_nodes_s>;
	t_items leafs_, nodes_;

	tree();

	static inline void inner_add_node(tree * tr, node * target, node * nd) {
		tr->nodes_.append(nd);
		nd->lt_ = target->rt_;
		target->rt_ = nd;
	}
	// left-to-right
	static void add_node_assoc_left(tree * tr, node * nd) {
		node * target = *tr->nodes_.items_;
		while( target->rt_->info_.prec_ > nd->info_.prec_search_ )
		target = target->rt_;

		inner_add_node(tr, target, nd);
	}
	// right-to-left
	static void add_node_assoc_right(tree * tr, node * nd) {
		node * target = *tr->nodes_.items_;
		while( target->rt_->info_.prec_ >= nd->info_.prec_search_ )
		target = target->rt_;

		inner_add_node(tr, target, nd);
	}
	static void add_leaf(tree * tr, node * nd) {
		tr->leafs_.append(nd);
		node * target = tr->nodes_.last(0);
		target->rt_ = nd;
	}

	void add_op(const also::t_pos & pos, const op_info_lite * op) {
		op->fn_adder_(this, new node{ op->info_, {op->type_, {pos, op->data_} } });
	}
};

struct scope {
	using t_items = ex::def_array<tree *, ex::del_pointer, def::ast_trees_r, def::ast_trees_s>;
	mod::instr i_first_, i_step_, i_last_;
	t_items trees_;

	static scope * make_function_scope();
	static scope * make_parentheses_scope();
	static scope * make_array_scope();
	static scope * make_map_scope();
	static scope * make_bracket_scope();

	tree * add_tree() {
		tree * tr = new tree;
		trees_.append(tr);
		return tr;
	}

	tree * last_tree() {
		if( trees_.count_ )
		return trees_.last(0);
		return add_tree();
	}

	void perform(prepare_data * pd);
};

struct fn_info {
	mod::fn_kind kind_ = mod::fk_common;
	mod::hfn_adder adder_;
	var::var_type var_type_;
	wtext name_;
	ex::ref<mod::fn_body, false> fn_body_;

	void put_fn(prepare_data * pd, space * spc, base_log * log);
};

struct body : public fn_info {
	using t_scopes = ex::def_array<scope *, ex::del_pointer, def::ast_scopes_r, def::ast_scopes_s>;
	using t_pos = ex::def_array<id, ex::del_plain, def::ast_pos_r, def::ast_pos_s>;

	bool is_plain_;
	t_scopes scopes_;
	t_pos scope_pos_, side_pos_;

	body(mod::module * md, bool is_plain = false) : is_plain_(is_plain) {
		adder_ = mod::fn_traits<mod::fk_common>::reg;
		var_type_ = &var::hcfg->t_null;
		scopes_.append(scope::make_function_scope() );
		scope_pos_.append(0);
		side_pos_.append(0);
		if( is_plain )
		fn_body_.h_ = &md->plain_;
		else
		fn_body_ = new mod::fn_body(md);
	}

	id add_scope_pos(scope * sc) {
		id ret = scopes_.count_;
		scope_pos_.append(ret);
		scopes_.append(sc);
		return ret;
	}
	void del_scope_pos(id n = 1) {
		scope_pos_.remove_last_n(n);
	}

	id add_side_pos() {
		id ret = fn_body_.h_->sides_.count_;
		side_pos_.append(ret);
		fn_body_.h_->sides_.append(new mod::side);
		return ret;
	}
	void del_side_pos(id n = 1) {
		side_pos_.remove_last_n(n);
	}

	scope * current_scope() {
		return scopes_.items_[ scope_pos_.last(0) ];
	}
	scope * first_scope() {
		return *scopes_.items_;
	}

	mod::side * current_side() {
		return fn_body_.h_->sides_.items_[ side_pos_.last(0) ];
	}
};

struct fn_call_places {
	struct place {
		wtext fn_name_;
		bool is_bk_;
		also::t_pos call_pos_;
		mod::module * mod_;
		const bool * actual_;
		mod::side * side_;
		id i_pos_; // instruction position
	};

	using t_fn_body_actual = ex::def_map<
		mod::fn_body *, bool, ex::map_del_plain, ex::map_del_plain, ex::cmp_std_plain,
		def::ast_fn_info_r, def::ast_fn_info_s
	>;
	using t_fn_global = ex::def_map<
		wtext, bool, ex::map_del_object, ex::map_del_plain, ex::cmp_std_plain,
		def::funcs_r, def::funcs_s
	>;
	using t_call_places = ex::def_array<place, ex::del_object, def::ast_fn_call_r, def::ast_fn_call_s>;

	t_fn_body_actual fn_body_actual_;
	t_fn_global fn_global_;
	t_call_places call_places_;

	bool * actual(mod::fn_body * fnb) {
		return &fn_body_actual_.append(fnb, true, ex::same_key::ignore)->val_;
	}
	void actual_not(mod::fn_body * fnb) {
		if( t_fn_body_actual::t_res_node res = fn_body_actual_.find_node(fnb) )
		res.pos_->val_ = false;
	}
	void global_add(const wtext & fn_name) {
		fn_global_.append(fn_name, true, ex::same_key::ignore);
	}
	bool global_check(const wtext & fn_name) const {
		return fn_global_.find_key(fn_name);
	}

	id place_add(const wtext & fn_name, mod::fn_body * fnb, bool is_bk, const also::t_pos & call_pos, mod::module * mod) {
		id ret = call_places_.count_;
		call_places_.append({ fn_name, is_bk, call_pos, mod, actual(fnb) });
		return ret;
	}
	void place_side_pos(id n, mod::side * sd, id i_pos) {
		place * pl = call_places_.items_ + n;
		pl->side_ = sd;
		pl->i_pos_ = i_pos;
	}

	void check_places(prepare_data * pd, base_log * log);
	void update_instructions(space * spc, base_log * log);
};

struct prepare_data {
	using fn_info_array = ex::def_array<fn_info, ex::del_object, def::ast_fn_info_r, def::ast_fn_info_s>;

	ex::ref<body, false> body_;
	ex::ref<mod::module, false> mod_;
	fn_info_array functions_;
	fn_call_places fn_call_places_;
	id error_count_ = 0;

	prepare_data(space * spc, const wtext & mod_path) {
		mod_ = new mod::module(mod_path, spc->mods_.arr_.count_);
	}

	void check(base_log * log) {
		fn_call_places_.check_places(this, log);
	}

	void put_fn() {
		if( body * bd = body_.h_; !bd->is_plain_ ) {
			functions_.append(*bd);
		}
	}

	void put_fn_all(space * spc, base_log * log) {
		for( fn_info & it : functions_ )
		it.put_fn(this, spc, log);
	}
	void put_mod(space * spc) {
		wtext mod_name = var::type_text::get_text(&mod_.h_->name_); // .value_.keep_->k_text()->tx_;
		spc->mods_.add(mod_name, mod_.h_);
		mod_.detach();
	}
	void put(space * spc, base_log * log) {
		put_mod(spc);
		put_fn_all(spc, log);
		fn_call_places_.update_instructions(spc, log);
	}

	void begin_plain(const also::t_pos & pos) {
		body * bd = new body(mod_.h_, true);
		body_ = bd;
		bd->fn_body_.h_->pos_ = pos;
	}
	void begin_fn_body(const also::t_pos & pos, const wtext & fn_name) {
		body * bd = new body(mod_.h_);
		body_ = bd;
		bd->fn_body_.h_->pos_ = pos;
		bd->name_ = fn_name;
		fn_call_places_.global_add(fn_name);
	}
	void end_fn_body() {
		body * bd = body_.h_;
		bd->first_scope()->perform(this);
		put_fn();
		body_ = nullptr;
	}

	tree * current_tree() {
		return body_.h_->current_scope()->last_tree();
	}

	scope ** get_scopes() {
		return body_.h_->scopes_.items_;
	}
};

template <class T>
struct array_iter {
	T * begin_, * end_;

	template <id N>
	array_iter(T (&arr)[N]) : begin_(arr), end_(arr + N) {}

	T * begin() const {
		return begin_;
	}
	T * end() const {
		return end_;
	}
};

struct actions {
	static void do_root(prepare_data * pd, tree * tr, node * parent, node * nd) {
		if( nd->rt_ )
		nd->rt_->info_.action_(pd, tr, nd, nd->rt_);
	}
	static node_info info_root() {
		return {do_root, prec_root, prec_root};
	}

	static inline void node_add_instr(prepare_data * pd, node * nd) {
		mod::side * sd = pd->body_.h_->current_side();
		sd->add_instr(nd->instr_);
	}

	// left-to-right
	static void do_node_assoc_left(prepare_data * pd, tree * tr, node * parent, node * nd) {
		nd->lt_->info_.action_(pd, tr, nd, nd->lt_);
		nd->rt_->info_.action_(pd, tr, nd, nd->rt_);
		node_add_instr(pd, nd);
	}
	// right-to-left
	static void do_node_assoc_right(prepare_data * pd, tree * tr, node * parent, node * nd) {
		nd->rt_->info_.action_(pd, tr, nd, nd->rt_);
		nd->lt_->info_.action_(pd, tr, nd, nd->lt_);
		node_add_instr(pd, nd);
	}

	static void do_none_assoc_left(prepare_data * pd, tree * tr, node * parent, node * nd) {
		nd->lt_->info_.action_(pd, tr, nd, nd->lt_);
		nd->rt_->info_.action_(pd, tr, nd, nd->rt_);
	}

	static void do_each_assoc_left(prepare_data * pd, tree * tr, node * parent, node * nd);

	static void do_fn_global_call(prepare_data * pd, tree * tr, node * parent, node * nd) {
		nd->lt_->info_.action_(pd, tr, nd, nd->lt_);
		nd->rt_->info_.action_(pd, tr, nd, nd->rt_);
		//
		mod::side * sd = pd->body_.h_->current_side();
		pd->fn_call_places_.place_side_pos(nd->info_.data_, sd, sd->instructions_.count_);
		node_add_instr(pd, nd);
	}

	template <n_node_kind Kind>
	static id calc_cmp_x_depth(node * nd) {
		id depth = 0;
		for( node * it = nd->lt_; it->info_.kind_ == Kind; it = it->lt_ )
		++depth;

		return depth;
	}

	template <class Instructions, n_node_kind Kind>
	static void do_cmp_x_assoc_left(prepare_data * pd, tree * tr, node * parent, node * nd) {
		// left
		nd->lt_->info_.action_(pd, tr, nd, nd->lt_);
		// right
		nd->rt_->info_.action_(pd, tr, nd, nd->rt_);
		//
		body * bd = pd->body_.h_;
		mod::side * sd = bd->current_side();
		if( parent->info_.kind_ == Kind ) {
			mod::instr tmp = nd->instr_;
			tmp.type_ = Instructions::turn_cmp_x(tmp.type_);
			tmp.params_.extra_ = bd->add_side_pos();
			sd->add_instr(tmp);
		} else {
			sd->add_instr(nd->instr_);
			if( id depth = calc_cmp_x_depth<Kind>(nd) )
			bd->del_side_pos(depth);
		}
	}

	static id calc_concat_depth(node * nd) {
		id depth = 1;
		for( node * it = nd->lt_; it->info_.kind_ == nk_concat; it = it->lt_ )
		++depth;

		return depth;
	}
	static void do_concat_assoc_left(prepare_data * pd, tree * tr, node * parent, node * nd);
	static void do_concat_assoc_right(prepare_data * pd, tree * tr, node * parent, node * nd);

	static bool is_node_dive(node * nd) {
		return nd->info_.kind_ == nk_dot || nd->info_.kind_ == nk_bracket;
	}
	static id calc_trees(scope ** scopes, node * nd) {
		id ret = scopes[nd->info_.data_]->trees_.count_;
		if( !ret )
		ret = 1;

		return ret;
	}
	static id calc_dive_depth(prepare_data * pd, node * nd) {
		scope ** scopes = pd->get_scopes();
		id depth = 0;
		bool go = true;
		while( go ) {
			if( nd->info_.kind_ == nk_dot )
			++depth;
			else if( nd->info_.kind_ != nk_bracket )
			go = false;

			if( go ) {
				if( nd->lt_->info_.kind_ == nk_bracket_args )
				depth += calc_trees(scopes, nd->lt_);

				nd = nd->rt_;
			}
		}
		if( nd->info_.kind_ == nk_bracket_args )
		depth += calc_trees(scopes, nd);

		return depth;
	}
	static void do_dot_get(prepare_data * pd, tree * tr, node * parent, node * nd);
	static void do_dot_set(prepare_data * pd, tree * tr, node * parent, node * nd);

	static void do_lazy_assoc_left(prepare_data * pd, tree * tr, node * parent, node * nd) {
		// left child
		nd->lt_->info_.action_(pd, tr, nd, nd->lt_);
		// prepare right
		body * bd = pd->body_.h_;
		nd->instr_.params_.extra_ = bd->add_side_pos();
		// right child
		nd->rt_->info_.action_(pd, tr, nd, nd->rt_);
		bd->del_side_pos();
		// current
		node_add_instr(pd, nd);
	}

	static void do_lazy_assoc_right(prepare_data * pd, tree * tr, node * parent, node * nd) {
		// right child
		nd->rt_->info_.action_(pd, tr, nd, nd->rt_);
		// prepare left
		body * bd = pd->body_.h_;
		nd->instr_.params_.extra_ = bd->add_side_pos();
		// left child
		nd->lt_->info_.action_(pd, tr, nd, nd->lt_);
		bd->del_side_pos();
		// current
		node_add_instr(pd, nd);
	}

	static void do_leaf(prepare_data * pd, tree * tr, node * parent, node * nd) {
		node_add_instr(pd, nd);
	}
	static void do_leaf_none(prepare_data * pd, tree * tr, node * parent, node * nd) {}
	// ()
	static void do_leaf_scope(prepare_data * pd, tree * tr, node * parent, node * nd) {
		scope * sc = pd->get_scopes()[nd->info_.data_];
		if( sc->trees_.count_ )
		sc->perform(pd);
		else
		node_add_instr(pd, nd);
	}
	// []
	static void do_leaf_array(prepare_data * pd, tree * tr, node * parent, node * nd) {
		/*body * bd = pd->body_.h_;
		scope * sc = bd->scopes_.items_[nd->info_.data_];*/
		scope * sc = pd->get_scopes()[nd->info_.data_];
		id count = sc->trees_.count_;
		nd->instr_.params_.data_ = count;
		node_add_instr(pd, nd);
		if( count )
		sc->perform(pd);
	}
	// ?
	static void do_leaf_then(prepare_data * pd, tree * tr, node * parent, node * nd);
	// ? |
	static void do_leaf_then_else(prepare_data * pd, tree * tr, node * parent, node * nd);
	static void turn_leaf_then_to_else(node * nd);

	static void do_leaf_while(prepare_data * pd, tree * tr, node * parent, node * nd);

	struct prefix_operator {
		static void do_leaf_assoc_left(prepare_data * pd, tree * tr, node * parent, node * nd);
		static void do_leaf_cmp_x_assoc_left(prepare_data * pd, tree * tr, node * parent, node * nd);
		static void do_leaf_concat_assoc_left(prepare_data * pd, tree * tr, node * parent, node * nd);
		static void do_leaf_concat_assoc_right(prepare_data * pd, tree * tr, node * parent, node * nd);
		static void do_leaf_lazy_assoc_left(prepare_data * pd, tree * tr, node * parent, node * nd);
	};

	static node_info info_leaf(hfn_action action = do_leaf, n_node_kind kind = nk_not_set) {
		return {action, prec_leaf, prec_leaf, kind};
	}

	using t_map_named_ops = ex::map<wtext, op_info_lite, ex::map_del_object, ex::map_del_object, ex::cmp_std_plain>;
	static array_iter<op_info> iter_op_assign();
	static array_iter<op_info> iter_op_assign_rt();
	static array_iter<op_info> iter_op();
	static array_iter<op_info> iter_prefix_op();
	static t_map_named_ops & map_named_ops();
	static t_map_named_ops & map_prefix_named_ops();
	static op_info_lite * op_colon(); // :
	static op_info_lite * op_dot_get(); // .
	static op_info_lite * op_dot_set(); // .
	static op_info_lite * op_bracket_get(); // []
	static op_info_lite * op_bracket_set(); // []
	static op_info_lite * op_type_of(); // a$
	static op_info_lite * op_then(); // ?
	static op_info_lite * op_each(); // each
	static op_info_lite * op_for(); // for
};

} // ns
} // ns
