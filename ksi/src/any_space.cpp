#include "space.h"

namespace ksi {
namespace var {

id type_fn::value_int(const any & a) {
	return a.value_.func_->id_;
}
id type_module::value_int(const any & a) {
	return a.value_.mod_->id_;
}

// type_fn
id type_fn::cmp_fn_x(const mod::base_func * v1, const any & v2) const {
	const mod::base_func * f2 = v2.value_.func_;
	return v1->type_ == f2->type_ ?
		ex::cmp_std_plain::compare(v1->id_, f2->id_) :
		ex::cmp::special_compare(v1->type_, f2->type_)
	;
}
void type_fn::out(const any & v, std::wostream & wo) const {
	wo << v.value_.func_->name_/*.value_.keep_->k_text()->tx_.h_->cs_*/;
}
void type_fn::dump(const any & v, std::wostream & wo, bool structured, id depth, map_keep * hmk) const {
	wo << v.value_.func_->name_/*.value_.keep_->k_text()->tx_.h_->cs_*/ << L'&';
}

// convert to fn
const mod::base_func * base_type::to_fn(const any & a, wtext & msg, space * spc) const {
	return hcfg->native_->fn_null_;
}
const mod::base_func * type_int::to_fn(const any & a, wtext & msg, space * spc) const {
	id pos = a.value_.int_;
	if( pos < 0 ) {
		msg = L"notice: Converting from $int to $fn with negative value.";
		return hcfg->native_->fn_null_;
	}
	mod::fn_map<mod::func_native> * mp = &hcfg->native_->fn_map_;
	if( pos >= mp->arr_.count_ ) {
		msg = L"notice: Converting from $int to $fn with too large value.";
		pos = mp->arr_.count_ -1;
	}
	return mp->arr_.items_[pos];
}
const mod::base_func * type_text::to_fn(const any & a, wtext & msg, space * spc) const {
	wtext tx = a.value_.keep_->k_text()->tx_;
	switch( tx.h_->cs_[0] ) {
	case 0:
		return hcfg->native_->fn_null_; break;
	case L'#':
		{
			mod::fn_map<mod::func_native> * mp = &hcfg->native_->fn_map_;
			if( ex::id_search_res res = mp->map_.find_key(tx) )
			return mp->arr_.items_[ mp->map_.sorted_.items_[res.pos_]->val_ ];
			else {
				msg = L"notice: Converting from $text to $fn with unknown native function name.";
				return hcfg->native_->fn_null_;
			}
		}
		break;
	case L'&':
		{
			id pos;
			if( ex::traits::find_char(tx, L'@', pos) ) {
				wtext mod_name = ex::traits::after_pos(tx, pos);
				tx = ex::traits::before_pos(tx, pos);
				if( space::t_modules::t_res res = spc->mods_.find_item(mod_name) ) {
					mod::module * md = res.pos_;
					if( ex::search_res<mod::func_mod *> res2 = md->fn_map_.find_item(tx) )
					return res2.pos_;
					msg = L"notice: Converting from $text to $fn with unknown function name.";
					return hcfg->native_->fn_null_;
				}
				msg = L"notice: Converting from $text to $fn with unknown module name.";
				return hcfg->native_->fn_null_;
			}
			mod::fn_map<mod::func> * mp = &spc->fn_map_;
			if( ex::id_search_res res = mp->map_.find_key(tx) )
			return mp->arr_.items_[ mp->map_.sorted_.items_[res.pos_]->val_ ];
			else {
				msg = L"notice: Converting from $text to $fn with unknown function name.";
				return hcfg->native_->fn_null_;
			}
		}
		break;
	}
	msg = L"notice: Converting from $text to $fn with wrong function name.";
	return hcfg->native_->fn_null_;
}

// convert from fn
bool type_fn::to_bool(const any & a, wtext & msg) const {
	return a.value_.func_ != hcfg->native_->fn_null_;
}
any type_fn::to_text(const any & a, wtext & msg) const {
	return a.value_.func_->name_;
}

// type_module
id type_module::cmp_mod_x(const mod::module * v1, const any & v2) const {
	return ex::cmp_std_plain::compare(v1->id_, v2.value_.mod_->id_);
}
void type_module::out(const any & v, std::wostream & wo) const {
	wo << v.value_.mod_->name_/*.value_.keep_->k_text()->tx_.h_->cs_*/;
}
void type_module::dump(const any & v, std::wostream & wo, bool structured, id depth, map_keep * hmk) const {
	wo << v.value_.mod_->name_/*.value_.keep_->k_text()->tx_.h_->cs_*/;
}

// convert to module
const mod::module * base_type::to_mod(const any & a, wtext & msg, space * spc) const {
	return spc->mods_.arr_.items_[0];
}
const mod::module * type_int::to_mod(const any & a, wtext & msg, space * spc) const {
	id pos = a.value_.int_;
	if( pos < 0 ) {
		msg = L"notice: Converting from $int to $module with negative value.";
		return spc->mods_.arr_.items_[0];
	}
	space::t_modules * mp = &spc->mods_;
	if( pos >= mp->arr_.count_ ) {
		msg = L"notice: Converting from $int to $module with too large value.";
		pos = mp->arr_.count_ -1;
	}
	return mp->arr_.items_[pos];
}
const mod::module * type_text::to_mod(const any & a, wtext & msg, space * spc) const {
	if( space::t_modules::t_res res = spc->mods_.find_item(a.value_.keep_->k_text()->tx_) )
	return res.pos_;
	msg = L"notice: Converting from $text to $module with unknown module name.";
	return spc->mods_.arr_.items_[0];
}

// convert from module
any type_module::to_text(const any & a, wtext & msg) const {
	return a.value_.mod_->name_;
}

// convert to type
var_type type_int::to_type(const any & a, wtext & msg, space * spc) const {
	id pos = a.value_.int_;
	if( pos < 0 ) {
		msg = L"notice: Converting from $int to $type with negative value.";
		return &hcfg->t_null;
	}
	if( pos >= spc->types_.arr_.count_ ) {
		msg = L"notice: Converting from $int to $type with too large value.";
		pos = spc->types_.arr_.count_ -1;
	}
	return spc->types_.arr_.items_[pos];
}
var_type type_text::to_type(const any & a, wtext & msg, space * spc) const {
	if( space::t_types::t_res res = spc->types_.find_item(a.value_.keep_->k_text()->tx_) )
	return res.pos_;
	msg = L"notice: Converting from $text to $type with unknown type name.";
	return &hcfg->t_null;
}

} // ns
} // ns
