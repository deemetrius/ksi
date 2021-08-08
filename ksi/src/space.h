#pragma once
#include "fn.h"

namespace ksi {
namespace mod {

struct module : ex::with_deleter<module> {
	using t_literals = ex::def_array<var::any, ex::del_object, def::literals_r, def::literals_s>;
	var::any name_;
	wtext path_;
	id id_;
	bool was_run_ = false;
	fn_map<func_mod> fn_map_;
	t_literals lit_;
	fn_body plain_;

	module(const wtext & path, id i) :
		name_( ex::implode({L"@", ex::to_wtext(i) }) ),
		path_(path),
		id_(i),
		fn_map_{this},
		plain_(this)
	{}

	var::any invoke(space * spc, mod::fn_space * caller, t_stack * stk, const also::t_pos & pos, base_log * log) {
		var::ref_var vars[plain_.var_names_.sorted_.count_];
		// fn space
		mod::fn_space fns = {vars, nullptr, pos};
		fns.prev_ = caller;
		// call
		plain_.call(spc, &fns, stk, log);
		was_run_ = true;
		return vars[0].h_->val_;
	}
};

} // ns

struct space {
	/* enum file_status { fs_ready, fs_loading, fs_parse_error };
	using t_files_map = ex::def_map<
		wtext, file_status, ex::map_del_object, ex::map_del_plain, ex::cmp_std_plain,
		def_modules_r, def_modules_s
	>; */
	using t_modules = ex::hive<mod::module *, ex::del_ex_pointer, def::modules_r, def::modules_s>;
	using t_types = ex::hive<var::var_type, var::del_custom_type, def::types_r, def::types_s>;
	// t_files_map				files_;
	t_types					types_;
	t_modules				mods_;
	mod::fn_map<mod::func>	fn_map_;

	space() {
		for( var::var_type it : var::hcfg->tc->types_ )
		types_.add(var::type_text::get_text(it->name_), it);

		mod::module * md = new mod::module(L"<none>", 0);
		mods_.add(L"@0", md);
		md->was_run_ = true;
	}

	id reg_type(var::base_type * tp) {
		id ret = types_.arr_.count_;
		tp->id_ = ret;
		types_.add(var::type_text::get_text(tp->name_), tp);
		return ret;
	}

	void first_run(mod::fn_space * caller, t_stack * stk, const also::t_pos & pos, base_log * log) {
		for( mod::module * it : mods_.arr_.get_rev_iter(1) )
		if( !it->was_run_ ) {
			var::any r = it->invoke(this, caller, stk, pos, log);
		}
	}
};

//

namespace mod {

enum fn_kind { fk_common, fk_non_static, fk_static };
using fn_reg_result = ex::search_res<fn_body *>;

template <fn_kind FK>
struct fn_traits {
	static fn_reg_result reg(space * spc, const wtext & name, var::var_type tp, fn_body * fnb) {
		module * md = fnb->mod_;
		func_mod * fn = md->fn_map_.obtain(name, &spc->fn_map_);
		if constexpr( FK == fk_common ) {
			fn->global_->over_.set_common(fnb);
			return fn->over_.set_common(fnb);
		} else if constexpr( FK == fk_non_static ) {
			fn->global_->over_.add(tp, fnb);
			return fn->over_.add(tp, fnb);
		} else if constexpr( FK == fk_static ) {
			fn->global_->over_.add_static(tp, fnb);
			return fn->over_.add_static(tp, fnb);
		}
	}
};

using hfn_adder = decltype(&fn_traits<fk_common>::reg);

} // ns
} // ns
