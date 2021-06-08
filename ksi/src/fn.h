#pragma once
#include "any.h"
#include "ksi_api.h"
#include <type_traits>

namespace ksi {

struct space;

namespace mod {

template <class T, template <class> class Closer = ex::map_del_plain>
struct over {
	using t_map = ex::def_map<
		var::var_type, T, ex::map_del_plain, Closer, var::cmp_any,
		def_over_r, def_over_s
	>;
	using pass = typename t_map::pass_val;
	using t_res = ex::search_res<T>;

	T common_ = nullptr;
	t_map non_static_, static_;

	~over() {
		if( common_ )
		Closer<T>::close(common_);
	}
	// returns: {true, fn} if not replaced
	t_res set_common(pass fn, bool force = false) {
		bool ret = true;
		if( force ) {
			Closer<T>::close(common_);
			common_ = fn;
			ret = false;
		} else {
			if( common_ )
			Closer<T>::close(fn);
			else {
				common_ = fn;
				ret = false;
			}
		}
		return {ret, common_};
	}
	static t_res inner_add(t_map & map, var::var_type tp, pass fn, bool force) {
		ex::id_search_res res = map.find_key(tp);
		typename t_map::cnode * nd = map.inner_insert_after(
			tp, fn,
			force ? ex::same_key::update : ex::same_key::ignore,
			map.in_end(), res
		);
		return {force ? false : res.found_, nd->val_};
	}
	t_res add(var::var_type tp, pass fn, bool force = false) {
		//non_static_.append(tp, fn, force ? ex::same_key::update : ex::same_key::ignore);
		return inner_add(non_static_, tp, fn, force);
	}
	t_res add_static(var::var_type tp, pass fn, bool force = false) {
		//static_.append(tp, fn, force ? ex::same_key::update : ex::same_key::ignore);
		return inner_add(static_, tp, fn, force);
	}
	T match(const var::any & a) const {
		if( a.type_ == &var::hcfg->t_type ) {
			if( ex::id_search_res res = static_.find_key(a.value_.type_) )
			return static_.sorted_.items_[res.pos_]->val_;
		}
		if( ex::id_search_res res = non_static_.find_key(a.type_) )
		return non_static_.sorted_.items_[res.pos_]->val_;
		return common_;
	}
};

struct module;
struct fn_space;
struct fn_body;

} // ns

struct t_stack {
	struct t_state {
		id n_items_, n_implode_;
	};

	struct for_implode : public ex::with_deleter<for_implode>, public ex::wtext_array {
		using ex::wtext_array::wtext_array;
	};
	using t_for_implode = ex::def_array<for_implode *, ex::del_ex_pointer, def_for_implode_r, def_for_implode_s>;
	using t_items = ex::def_array<var::any, ex::del_object, def_stack_r, def_stack_s>;

	t_items items_;
	t_for_implode for_implode_;

	t_state state_get() const {
		return {items_.count_, for_implode_.count_};
	}
	void state_restore(const t_state & st) {
		if( id diff = items_.count_ - st.n_items_; diff > 0 )
		items_.remove_last_n(diff);

		if( id diff = for_implode_.count_ - st.n_implode_; diff > 0 )
		for_implode_.remove_last_n(diff);
	}
};

namespace mod {

struct base_func : ex::with_deleter<base_func> {
	enum n_type { f_native = 1, f_global, f_module };
	var::any name_;
	id id_;
	n_type type_;

	base_func(const var::any & name, id i, n_type type) : name_(name), id_(i), type_(type) {}

	virtual ~base_func() {}
	virtual void call_2(space * spc, fn_space * caller, t_stack * stk, const also::t_pos & pos, base_log * log) const = 0;
	virtual void call_3(space * spc, fn_space * caller, t_stack * stk, const also::t_pos & pos, base_log * log) const = 0;
	virtual void call_2_bk(space * spc, fn_space * caller, t_stack * stk, const also::t_pos & pos, base_log * log) const = 0;
	virtual var::any operator () (
		space * spc, fn_space * caller, t_stack * stk, const also::t_pos & pos, base_log * log,
		const var::any & v1, const var::any & v2, const var::any & v3 = var::n_null::val
	) const = 0;
};

struct fn_space {
	var::ref_var * vars_;
	const base_func * func_;
	also::t_pos pos_;
	module * mod_ = nullptr;
	var::any * args_ = nullptr;
	fn_body * fn_body_ = nullptr;
	fn_space * prev_ = nullptr;

	wtext file_path();
};

// instructions

struct instr_data {
	also::t_pos pos_;
	id data_ = 0, extra_ = 0, mod_ = 0, also_ = 0;
};
using hfn_instr = void (*)(space * spc, fn_space * fns, t_stack * stk, base_log * log, const instr_data & params);
struct instr_type {
	wtext name_;
	hfn_instr perform_;
	bool empty_ = false;
};
struct instr {
	const instr_type * type_;
	instr_data params_;
};

inline std::wostream & operator << (std::wostream & wo, const instr & ins) {
	wo	<< ins.type_->name_ << L" : "
		<< ins.params_.data_ << L", "
		<< ins.params_.extra_ << L", "
		<< ins.params_.mod_ << L", "
		<< ins.params_.also_
	;
	return wo;
}

// side
struct side {
	using t_items = ex::def_array<instr, ex::del_plain_struct, def_instr_r, def_instr_s>;
	t_items instructions_;

	void add_instr(const instr & ins) {
		if( !ins.type_->empty_ )
		instructions_.append(ins);
	}

	void call(space * spc, fn_space * fns, t_stack * stk, base_log * log) {
		for( const instr & it : instructions_ )
		it.type_->perform_(spc, fns, stk, log, it.params_);
	}
};

//

struct fn_body : ex::with_deleter<fn_body> {
	using t_var_names = ex::def_map<
		wtext, id, ex::map_del_object, ex::map_del_plain, ex::cmp_std_plain,
		def_var_names_r, def_var_names_s
	>;
	using t_sides = ex::def_array<side *, ex::del_pointer, def_sides_r, def_sides_s>;

	also::t_pos pos_;
	module * mod_;
	t_var_names var_names_;
	t_sides sides_;

	fn_body(module * md) : mod_(md) {
		reg_var(L"ret");
		sides_.append(new side);
	}
	id reg_var(const wtext & name) {
		id num = var_names_.sorted_.count_;
		return var_names_.append(name, num, ex::same_key::ignore)->val_;
	}

	void call(space * spc, fn_space * fns, t_stack * stk, base_log * log) {
		fns->mod_ = mod_;
		fns->fn_body_ = this;
		// instructions
		side * v_side = *sides_.items_;
		for( const instr & it : v_side->instructions_ )
		it.type_->perform_(spc, fns, stk, log, it.params_);
	}
};

//using fn_native = void (*)(space * spc, fn_space * fns, t_stack * stk, base_log * log);
struct fn_native {
	fn_native_simple fn_ = nullptr;
	fn_native_can_throw fne_ = nullptr;

	fn_native(std::nullptr_t n = nullptr) {}
	fn_native(fn_native_can_throw fne) : fn_(var::hcfg->api_->fn_native_wrap_), fne_(fne) {}
	fn_native(fn_native_simple fn) : fn_(fn) {}

	inline void operator ()(space * spc, fn_space * fns, t_stack * stk, base_log * log) const {
		fn_(fne_, spc, fns, stk, log);
	}
	operator bool () const {
		return static_cast<bool>(fn_);
	}
};

template <id N, id V, id ... P>
struct f_caller {
	template <class F>
	static inline void call_user(
		const F * fun, space * spc, fn_space * caller, t_stack * stk,
		const also::t_pos & pos, base_log * log
	) {
		if( fn_body * fnb = fun->over_.match(stk->items_.last(V) ) ) {
			// get from stack
			var::ref_var vars[fnb->var_names_.sorted_.count_] = {var::any(), stk->items_.last(P) ...};
			stk->items_.remove_last_n(N);
			// fn space
			fn_space fns = {vars, fun, pos};
			fns.prev_ = caller;
			// call
			fnb->call(spc, &fns, stk, log);
			// result
			stk->items_.append(vars[0].h_->val_);
		} else {
			stk->items_.remove_last_n(N -1);
			stk->items_.last(0) = var::n_null::val;
		}
	}

	template <class F>
	static inline void call_native(
		const F * fun, space * spc, fn_space * caller, t_stack * stk,
		const also::t_pos & pos, base_log * log
	) {
		if( fn_native fnn = fun->over_.match(stk->items_.last(V) ) ) {
			// get from stack
			var::any args[4] = {var::n_null::val, stk->items_.last(P) ...};
			stk->items_.remove_last_n(N);
			// fn space
			fn_space fns = {nullptr, fun, pos};
			fns.args_ = args;
			if( caller )
			fns.mod_ = caller->mod_;
			fns.prev_ = caller;
			// call
			fnn(spc, &fns, stk, log);
			// result
			stk->items_.append(args[0]);
		} else {
			stk->items_.remove_last_n(N -1);
			stk->items_.last(0) = var::n_null::val;
		}
	}
};

using f_caller_2 = f_caller<2, 1, 1, 0>;
using f_caller_3 = f_caller<3, 1, 1, 2, 0>;
using f_caller_2_bk = f_caller<2, 0, 0, 1>;

struct f_caller_flat {
	template <class F>
	static inline var::any call_user(
		const F * fun, space * spc, fn_space * caller, t_stack * stk,
		const also::t_pos & pos, base_log * log,
		const var::any & v1, const var::any & v2, const var::any & v3
	) {
		if( fn_body * fnb = fun->over_.match(v1) ) {
			// vars
			var::ref_var vars[fnb->var_names_.sorted_.count_] = {var::any(), v1, v2, v3};
			// fn space
			fn_space fns = {vars, fun, pos};
			fns.prev_ = caller;
			// call
			fnb->call(spc, &fns, stk, log);
			// result
			return vars[0].h_->val_;
		}
		return var::n_null::val;
	}

	template <class F>
	static inline var::any call_native(
		const F * fun, space * spc, fn_space * caller, t_stack * stk,
		const also::t_pos & pos, base_log * log,
		const var::any & v1, const var::any & v2, const var::any & v3
	) {
		if( fn_native fnn = fun->over_.match(v1) ) {
			// args
			var::any args[4] = {var::n_null::val, v1, v2, v3};
			// fn space
			fn_space fns = {nullptr, fun, pos};
			fns.args_ = args;
			if( caller )
			fns.mod_ = caller->mod_;
			fns.prev_ = caller;
			// call
			fnn(spc, &fns, stk, log);
			// result
			return args[0];
		}
		return var::n_null::val;
	}
};

struct func : public base_func {
	over<fn_body *, ex::map_del_plain> over_;

	func(const var::any & name, id i) : base_func(name, i, f_global) {}

	void call_2(space * spc, fn_space * caller, t_stack * stk, const also::t_pos & pos, base_log * log) const override {
		f_caller_2::call_user(this, spc, caller, stk, pos, log);
	}
	void call_3(space * spc, fn_space * caller, t_stack * stk, const also::t_pos & pos, base_log * log) const override {
		f_caller_3::call_user(this, spc, caller, stk, pos, log);
	}
	void call_2_bk(space * spc, fn_space * caller, t_stack * stk, const also::t_pos & pos, base_log * log) const override {
		f_caller_2_bk::call_user(this, spc, caller, stk, pos, log);
	}
	var::any operator () (
		space * spc, fn_space * caller, t_stack * stk,
		const also::t_pos & pos, base_log * log,
		const var::any & v1, const var::any & v2, const var::any & v3 = var::n_null::val
	) const override {
		return f_caller_flat::call_user(this, spc, caller, stk, pos, log, v1, v2, v3);
	}
};

struct func_mod : public base_func {
	over<fn_body *, ex::map_del_ex_pointer> over_;
	func * global_ = nullptr;

	func_mod(const var::any & name, id i) : base_func(name, i, f_module) {}

	void call_2(space * spc, fn_space * caller, t_stack * stk, const also::t_pos & pos, base_log * log) const override {
		f_caller_2::call_user(this, spc, caller, stk, pos, log);
	}
	void call_3(space * spc, fn_space * caller, t_stack * stk, const also::t_pos & pos, base_log * log) const override {
		f_caller_3::call_user(this, spc, caller, stk, pos, log);
	}
	void call_2_bk(space * spc, fn_space * caller, t_stack * stk, const also::t_pos & pos, base_log * log) const override {
		f_caller_2_bk::call_user(this, spc, caller, stk, pos, log);
	}
	var::any operator () (
		space * spc, fn_space * caller, t_stack * stk,
		const also::t_pos & pos, base_log * log,
		const var::any & v1, const var::any & v2, const var::any & v3 = var::n_null::val
	) const override {
		return f_caller_flat::call_user(this, spc, caller, stk, pos, log, v1, v2, v3);
	}
};

struct func_native : public base_func {
	over<fn_native, ex::map_del_plain> over_;

	func_native(const var::any & name, id i) : base_func(name, i, f_native) {}

	void call_2(space * spc, fn_space * caller, t_stack * stk, const also::t_pos & pos, base_log * log) const override {
		f_caller_2::call_native(this, spc, caller, stk, pos, log);
	}
	void call_3(space * spc, fn_space * caller, t_stack * stk, const also::t_pos & pos, base_log * log) const override {
		f_caller_3::call_native(this, spc, caller, stk, pos, log);
	}
	void call_2_bk(space * spc, fn_space * caller, t_stack * stk, const also::t_pos & pos, base_log * log) const override {
		f_caller_2_bk::call_native(this, spc, caller, stk, pos, log);
	}
	var::any operator () (
		space * spc, fn_space * caller, t_stack * stk,
		const also::t_pos & pos, base_log * log,
		const var::any & v1, const var::any & v2, const var::any & v3 = var::n_null::val
	) const override {
		return f_caller_flat::call_native(this, spc, caller, stk, pos, log, v1, v2, v3);
	}
};

struct base_none {};
struct base_with_mod {
	module * mod_ = nullptr;

	wtext mod_name() const;
};

template <class T>
struct fn_map
: public std::conditional_t<std::is_same_v<T, func_mod>, base_with_mod, base_none>
, public also::hive<T *, ex::del_ex_pointer, def_funcs_r, def_funcs_s>
{
	T * obtain(const wtext & name, fn_map<func> * global = nullptr) {
		ex::id_search_res res = this->map_.find_key(name);
		T * ret;
		if( res )
		ret = this->arr_.items_[ this->map_.sorted_.items_[res.pos_]->val_ ];
		else {
			id num = this->arr_.count_;
			if constexpr( std::is_same_v<T, func_mod> )
			ret = new T( ex::implode({ name, this->mod_name() }), num);
			else
			ret = new T(name, num);
			this->inner_add(name, ret, res);
			if constexpr( std::is_same_v<T, func_mod> ) {
				if( global )
				ret->global_ = global->obtain(name);
			}
		}
		return ret;
	}
};

struct native_config {
	fn_map<func_native> fn_map_;
	func_native * fn_null_;

	native_config();
};

native_config * inst_native_config();

} // ns
} // ns
