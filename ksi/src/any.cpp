#include "any.h"
#include <errno.h>

namespace ksi {
namespace var {

const config * hcfg = nullptr;

double type_float::inf	= 1.0 / 0.0;
double type_float::inf_	= -1.0 / 0.0;
double type_float::nan	= 0.0 / 0.0;

any::any(keep_array * ka) : type_(&hcfg->t_array) {
	value_.keep_ = ka;
}
any::any(keep_map * km) : type_(&hcfg->t_map) {
	value_.keep_ = km;
}
any::any(ref_var * link) : type_(&hcfg->t_array) {
	value_.keep_ = new keep_array(link);
}

void base_type::native_set_name(const any * name, type_config * tc) {
	name_ = name;
	tc->map_types_.append(/*name->value_.keep_->k_text()->tx_*/type_text::get_text(name), this, ex::same_key::update);
}

config::config(const api * v_api) :
tc( base_type::type_config::instance() ),
t_null(tc),
t_any(tc),
t_number(tc),
t_bool(tc),
t_int(tc),
t_float(tc),
t_type(tc),
t_text(tc),
t_fn(tc),
t_module(tc),
t_array(tc, false),
t_map(tc, false) {
	hcfg = this;
	t_int	.is_map_key_ = true;
	t_type	.is_map_key_ = true;
	t_text	.is_map_key_ = true;
	values = values_config::instance();
	t_null		.native_set_name(&values->cn_null	, tc);
	t_any		.native_set_name(&values->cn_any	, tc);
	t_number	.native_set_name(&values->cn_number	, tc);
	t_bool		.native_set_name(&values->cn_bool	, tc);
	t_int		.native_set_name(&values->cn_int	, tc);
	t_float		.native_set_name(&values->cn_float	, tc);
	t_type		.native_set_name(&values->cn_type	, tc);
	t_text		.native_set_name(&values->cn_text	, tc);
	t_fn		.native_set_name(&values->cn_fn		, tc);
	t_module	.native_set_name(&values->cn_module	, tc);
	t_array		.native_set_name(&values->cn_array	, tc);
	t_map		.native_set_name(&values->cn_map	, tc);
	native_ = mod::inst_native_config();
	api_ = v_api;
}
#ifdef KSI_LIB
const config * get_config() {
	return hcfg;
}
#else
void set_config(const config * h) {
	hcfg = h;
}
#endif // KSI_LIB

// compare

// base
id base_type::compare(const any & v1, const any & v2) const {
	return ex::cmp_std_plain::compare(v1.type_->id_, v2.type_->id_);
}
id base_type::cmp_bool_x(bool v1, const any & v2) const {
	return ex::cmp::special_compare(hcfg->t_bool.id_, v2.type_->id_);
}
id base_type::cmp_int_x(id v1, const any & v2) const {
	return ex::cmp::special_compare(hcfg->t_int.id_, v2.type_->id_);
}
id base_type::cmp_float_x(real v1, const any & v2) const {
	return ex::cmp::special_compare(hcfg->t_float.id_, v2.type_->id_);
}
id base_type::cmp_type_x(const base_type * v1, const any & v2) const {
	return ex::cmp::special_compare(hcfg->t_type.id_, v2.type_->id_);
}
id base_type::cmp_text_x(const wtext & v1, const any & v2) const {
	return ex::cmp::special_compare(hcfg->t_text.id_, v2.type_->id_);
}
id base_type::cmp_fn_x(const mod::base_func * v1, const any & v2) const {
	return ex::cmp::special_compare(hcfg->t_fn.id_, v2.type_->id_);
}
id base_type::cmp_mod_x(const mod::module * v1, const any & v2) const {
	return ex::cmp::special_compare(hcfg->t_module.id_, v2.type_->id_);
}

// null
id type_null::compare(const any & v1, const any & v2) const {
	return v2.type_->cmp_null_x();
}

// bool
id type_bool::compare(const any & v1, const any & v2) const {
	return v2.type_->cmp_bool_x(v1.value_.bool_, v2);
}
id type_bool::cmp_bool_x(bool v1, const any & v2) const {
	return ex::cmp_std_plain::compare(v1, v2.value_.bool_);
}

// int
id type_int::compare(const any & v1, const any & v2) const {
	return v2.type_->cmp_int_x(v1.value_.int_, v2);
}
id type_int::cmp_int_x(id v1, const any & v2) const {
	return ex::cmp_std_plain::compare(v1, v2.value_.int_);
}
id type_int::cmp_float_x(real v1, const any & v2) const {
	return type_float::is_nan(v1) ? ex::cmp::less : ex::cmp_std_plain::compare(v1, v2.value_.int_);
}

// float
id type_float::compare(const any & v1, const any & v2) const {
	return v2.type_->cmp_float_x(v1.value_.float_, v2);
}
id type_float::cmp_int_x(id v1, const any & v2) const {
	return is_nan(v2.value_.float_) ? ex::cmp::more : ex::cmp_std_plain::compare(v1, v2.value_.float_);
}
id type_float::cmp_float_x(real v1, const any & v2) const {
	bool nan_v2 = is_nan(v2.value_.float_);
	return is_nan(v1) ?
		(nan_v2 ? ex::cmp::equal : ex::cmp::less) :
		(nan_v2 ? ex::cmp::more : ex::cmp_std_plain::compare(v1, v2.value_.float_) )
	;
}

// type
id type_type::compare(const any & v1, const any & v2) const {
	return v2.type_->cmp_type_x(v1.value_.type_, v2);
}
id type_type::cmp_type_x(const base_type * v1, const any & v2) const {
	return ex::cmp_std_plain::compare(v1->id_, v2.value_.type_->id_);
}

// text
id type_text::compare(const any & v1, const any & v2) const {
	return v2.type_->cmp_text_x(v1.value_.keep_->k_text()->tx_, v2);
}
id type_text::cmp_text_x(const wtext & v1, const any & v2) const {
	return ex::cmp_std_plain::compare(v1.h_->cs_, v2.value_.keep_->k_text()->tx_.h_->cs_);
}

// fn
id type_fn::compare(const any & v1, const any & v2) const {
	return v2.type_->cmp_fn_x(v1.value_.func_, v2);
}

// module
id type_module::compare(const any & v1, const any & v2) const {
	return v2.type_->cmp_mod_x(v1.value_.mod_, v2);
}

// base_ref
id base_ref::compare(const any & v1, const any & v2) const {
	return v1.type_ == v2.type_ ?
		ex::cmp_std_plain::compare(v1.value_.keep_, v2.value_.keep_) :
		ex::cmp::special_compare(v1.type_->id_, v2.type_->id_)
	;
}

// out

void type_bool::out(const any & v, std::wostream & wo) const {
	wo << v.value_.bool_;
}
void type_int::out(const any & v, std::wostream & wo) const {
	wo << v.value_.int_;
}
void type_float::out(const any & v, std::wostream & wo) const {
	wo << std::noshowpoint << v.value_.float_;
}
void type_type::out(const any & v, std::wostream & wo) const {
	wo << type_text::get_text(v.value_.type_->name_).h_->cs_;
}
void type_text::out(const any & v, std::wostream & wo) const {
	wo << v.value_.keep_->k_text()->tx_.h_->cs_;
}
void type_array::out(const any & v, std::wostream & wo) const {
	wo << L"array*" << /*v.value_.keep_->k_array()->items_.count_*/value_int(v);
}
void type_map::out(const any & v, std::wostream & wo) const {
	wo << L"map*" << value_int(v);
}

// dump

void type_null::dump(const any & v, std::wostream & wo, bool structured, id depth, map_keep * hmk) const {
	wo << L'#';
}
void type_bool::dump(const any & v, std::wostream & wo, bool structured, id depth, map_keep * hmk) const {
	wo << L'#' << v.value_.bool_;
}
void type_int::dump(const any & v, std::wostream & wo, bool structured, id depth, map_keep * hmk) const {
	id num = v.value_.int_;
	if( num < 0 ) {
		if( num > min )
		wo << (- num);
		else
		wo << (static_cast<uid>(max) +1);
		wo << L'_';
	} else
	wo << num;
}
void type_float::dump(const any & v, std::wostream & wo, bool structured, id depth, map_keep * hmk) const {
	real f = v.value_.float_;
	if( is_nan(f) )
	wo << L"$float.nan#";
	else if( f == inf )
	wo << L"$float.inf#";
	else if( f == inf_ )
	wo << L"$float.inf_#";
	else if( f == 0.0 )
	wo << L"0.0";
	else {
		if( f == std::trunc(f) )
		wo << std::showpoint;
		else
		wo << std::noshowpoint;
		if( f < 0 )
		wo << (- f) << L'_';
		else
		wo << f;
	}
}
void type_type::dump(const any & v, std::wostream & wo, bool structured, id depth, map_keep * hmk) const {
	wo << L'$' << *v.value_.type_->name_;
}
void type_text::dump(const any & v, std::wostream & wo, bool structured, id depth, map_keep * hmk) const {
	ex::traits::inner_escape(v.value_.keep_->k_text()->tx_, wo, true);
}
void type_array::inner_dump(const any & v, std::wostream & wo, bool structured, id depth, map_keep * hmk) const {
	wo << L'[';
	keep_array::t_items * items = &v.value_.keep_->k_array()->items_;
	id depth_1 = depth +1;
	if( structured ) {
		for( ref_var & it : *items ) {
			wo << std::endl;
			for( id i = 0; i < depth; ++i )
			wo << L'\t';
			it.h_->val_.type_->dump(it.h_->val_, wo, structured, depth_1, hmk);
		}
		if( items->count_ ) {
			wo << std::endl;
			for( id i = 0, end = depth -1; i < end; ++i )
			wo << L'\t';
		}
	} else {
		bool is_first = true;
		for( ref_var & it : *items ) {
			if( is_first )
			is_first = false;
			else
			wo << L", ";
			it.h_->val_.type_->dump(it.h_->val_, wo, structured, depth_1, hmk);
		}
	}
	wo << L']';
}
void type_array::dump(const any & v, std::wostream & wo, bool structured, id depth, map_keep * hmk) const {
	base_keep * hk = v.value_.keep_;
	if( hmk ) {
		if( hmk->find_key(hk) )
		wo << L"(recursion)";
		else {
			hmk->append(hk, true, ex::same_key::ignore);
			inner_dump(v, wo, structured, depth, hmk);
		}
	} else {
		map_keep mk;
		mk.append(hk, true, ex::same_key::ignore);
		inner_dump(v, wo, structured, depth, &mk);
	}
}
void type_map::inner_dump(const any & v, std::wostream & wo, bool structured, id depth, map_keep * hmk) const {
	wo << L'{';
	keep_map::t_items * items = v.value_.keep_->k_map()->ref_.h_;
	id depth_1 = depth +1;
	if( structured ) {
		for( keep_map::t_items::cnode * it : items->items_ ) {
			wo << std::endl;
			for( id i = 0; i < depth; ++i )
			wo << L'\t';
			it->key_.type_->dump(it->key_, wo, structured, depth_1, hmk);
			wo << L" : ";
			it->val_.h_->val_.type_->dump(it->val_.h_->val_, wo, structured, depth_1, hmk);
		}
		if( items->sorted_.count_ ) {
			wo << std::endl;
			for( id i = 0, end = depth -1; i < end; ++i )
			wo << L'\t';
		}
	} else {
		bool is_first = true;
		for( keep_map::t_items::cnode * it : items->items_ ) {
			if( is_first )
			is_first = false;
			else
			wo << L", ";
			it->key_.type_->dump(it->key_, wo, structured, depth_1, hmk);
			wo << L" : ";
			it->val_.h_->val_.type_->dump(it->val_.h_->val_, wo, structured, depth_1, hmk);
		}
	}
	wo << L'}';
}
void type_map::dump(const any & v, std::wostream & wo, bool structured, id depth, map_keep * hmk) const {
	base_keep * hk = v.value_.keep_;
	if( hmk ) {
		if( hmk->find_key(hk) )
		wo << L"(recursion)";
		else {
			hmk->append(hk, true, ex::same_key::ignore);
			inner_dump(v, wo, structured, depth, hmk);
		}
	} else {
		map_keep mk;
		mk.append(hk, true, ex::same_key::ignore);
		inner_dump(v, wo, structured, depth, &mk);
	}
}

// convert

any base_type::convert_from(const any & a, wtext & msg, space * spc) const {
	if( a.type_ == this )
	return a;
	return any();
}
any type_any::convert_from(const any & a, wtext & msg, space * spc) const {
	return a;
}
any type_number::convert_from(const any & a, wtext & msg, space * spc) const {
	return a.type_->to_number(a, msg);
}
any type_bool::convert_from(const any & a, wtext & msg, space * spc) const {
	return a.type_->to_bool(a, msg);
}
any type_int::convert_from(const any & a, wtext & msg, space * spc) const {
	return a.type_->to_int(a, msg);
}
any type_float::convert_from(const any & a, wtext & msg, space * spc) const {
	return a.type_->to_float(a, msg);
}
any type_type::convert_from(const any & a, wtext & msg, space * spc) const {
	return a.type_->to_type(a, msg, spc);
}
any type_text::convert_from(const any & a, wtext & msg, space * spc) const {
	return a.type_->to_text(a, msg);
}
any type_fn::convert_from(const any & a, wtext & msg, space * spc) const {
	return a.type_->to_fn(a, msg, spc);
}
any type_module::convert_from(const any & a, wtext & msg, space * spc) const {
	return a.type_->to_mod(a, msg, spc);
}
any type_array::convert_from(const any & a, wtext & msg, space * spc) const {
	return a.type_->to_array(a, msg);
}
any type_map::convert_from(const any & a, wtext & msg, space * spc) const {
	return a.type_->to_map(a, msg);
}

// to bool
bool base_type::to_bool(const any & a, wtext & msg) const {
	return false;
}
bool type_bool::to_bool(const any & a, wtext & msg) const {
	return a.value_.bool_;
}
bool type_float::to_bool(const any & a, wtext & msg) const {
	return !(is_nan(a.value_.float_) || a.value_.float_ == 0.0);
}
bool type_type::to_bool(const any & a, wtext & msg) const {
	return a.value_.type_ != &hcfg->t_null;
}
bool type_text::to_bool(const any & a, wtext & msg) const {
	return !a.value_.keep_->k_text()->tx_.empty();
}

// to int
id base_type::to_int(const any & a, wtext & msg) const {
	return 0;
}
id type_bool::to_int(const any & a, wtext & msg) const {
	return a.value_.bool_;
}
id type_float::to_int(const any & a, wtext & msg) const {
	real f = a.value_.float_;
	if( is_nan(f) ) {
		msg = L"notice: Converting from $float NaN to $int.";
		return 0;
	}
	if( f > 0.0 && f > type_int::max ) {
		msg = L"notice: Converting from $float to $int beyond max value.";
		return type_int::max;
	}
	if( f < 0.0 && f < type_int::min ) {
		msg = L"notice: Converting from $float to $int beyond min value.";
		return type_int::min;
	}
	return std::llround(f);
}
id type_text::to_int(const any & a, wtext & msg) const {
	errno = 0;
	id ret = wcstoll(
		a.value_.keep_->k_text()->tx_.h_->cs_,
		nullptr, 10
	);
	if( errno == ERANGE ) {
		errno = 0;
		msg = L"notice: Converting from $text to $int beyond its limits.";
	}
	return ret;
}

// to float
real base_type::to_float(const any & a, wtext & msg) const {
	return 0.0;
}
real type_bool::to_float(const any & a, wtext & msg) const {
	return static_cast<real>(a.value_.bool_);
}
real type_float::to_float(const any & a, wtext & msg) const {
	return a.value_.float_;
}
real type_text::to_float(const any & a, wtext & msg) const {
	errno = 0;
	real ret = WCS_TO_REAL(
		a.value_.keep_->k_text()->tx_.h_->cs_,
		nullptr
	);
	if( errno == ERANGE ) {
		errno = 0;
		msg = L"notice: Converting from $text to $float beyond its limits.";
	}
	return ret;
}

// to type
var_type base_type::to_type(const any & a, wtext & msg, space * spc) const {
	return a.type_;
}
var_type type_type::to_type(const any & a, wtext & msg, space * spc) const {
	return a.value_.type_;
}

// to text
any base_type::to_text(const any & a, wtext & msg) const {
	return hcfg->values->c_empty;
}
any type_bool::to_text(const any & a, wtext & msg) const {
	return a.value_.bool_ ? hcfg->values->c_one : hcfg->values->c_zero;
}
any type_int::to_text(const any & a, wtext & msg) const {
	return ex::to_wtext(a.value_.int_);
}
any type_float::to_text(const any & a, wtext & msg) const {
	return ex::to_wtext(a.value_.float_);
}
any type_type::to_text(const any & a, wtext & msg) const {
	return *a.value_.type_->name_;
}
any type_text::to_text(const any & a, wtext & msg) const {
	return a;
}
any type_array::to_text(const any & a, wtext & msg) const {
	return ex::implode({L"array*", ex::to_wtext(/*a.value_.keep_->k_array()->items_.count_*/ value_int(a) ) });
}
any type_map::to_text(const any & a, wtext & msg) const {
	return ex::implode({L"map*", ex::to_wtext(/*a.value_.keep_->k_map()->ref_.h_->sorted_.count_*/ value_int(a) ) });
}

// to fn
const mod::base_func * type_fn::to_fn(const any & a, wtext & msg, space * spc) const {
	return a.value_.func_;
}

// to module
const mod::module * type_module::to_mod(const any & a, wtext & msg, space * spc) const {
	return a.value_.mod_;
}

// to array
any base_type::to_array(const any & a, wtext & msg) const {
	return new keep_array{a};
}
any type_null::to_array(const any & a, wtext & msg) const {
	return new keep_array;
}
any type_array::to_array(const any & a, wtext & msg) const {
	return a;
}
keep_array::keep_array(const keep_map * km) : items_(km->ref_.h_->sorted_.count_ + def_array_s, def_array_s) {
	for( const keep_map::t_items::cnode * it : km->ref_.h_->items_ )
	items_.append_obj<const any &>(it->val_.h_->val_);
}
any type_map::to_array(const any & a, wtext & msg) const {
	return new keep_array(a.value_.keep_->k_map() );
}

// to map
any base_type::to_map(const any & a, wtext & msg) const {
	return new keep_map{ {n_null::val, a} };
}
any type_null::to_map(const any & a, wtext & msg) const {
	return new keep_map;
}
any type_array::to_map(const any & a, wtext & msg) const {
	return new keep_map(a.value_.keep_->k_array() );
}
any type_map::to_map(const any & a, wtext & msg) const {
	return a;
}

// to number
any base_type::to_number(const any & a, wtext & msg) const {
	return 0LL;
}
any type_bool::to_number(const any & a, wtext & msg) const {
	return static_cast<id>(a.value_.bool_);
}
any type_float::to_number(const any & a, wtext & msg) const {
	return a;
}
any type_text::to_number(const any & a, wtext & msg) const {
	const Char * cs = a.value_.keep_->k_text()->tx_.h_->cs_;
	Char * end_i;
	errno = 0;
	id ret_i = wcstoll(cs, &end_i, 10);
	auto ern_i = errno;
	Char * end_f;
	errno = 0;
	real ret_f = WCS_TO_REAL(cs, &end_f);
	if( end_i == end_f && ern_i != ERANGE )
	return ret_i;
	if( errno == ERANGE )
	msg = L"notice: Converting from $text to $float beyond its limits.";
	return ret_f;
}

// element_get

any base_type::element_get(const any & a, const any & key) const {
	return n_null::val;
}
any type_array::element_get(const any & a, const any & key) const {
	return a.value_.keep_->k_array()->get(key);
}
any type_map::element_get(const any & a, const any & key) const {
	return a.value_.keep_->k_map()->get(key);
}

// element_set

bool base_type::element_set(any & a_link, const any & key, keep_array * ka, wtext & msg) const {
	ref_var * link = ka->any_link_get(), * new_link;
	if( key.type_ == &hcfg->t_bool ) {
		keep_map * km;
		*link = km = new keep_map{ {n_null::val, n_null::val} };
		new_link = &km->ref_.h_->sorted_.items_[0]->val_;
	} else if( key.type_->is_map_key_ ) {
		keep_map * km;
		*link = km = new keep_map{ {key, n_null::val} };
		new_link = &km->ref_.h_->sorted_.items_[0]->val_;
	} else {
		keep_array * new_ka;
		*link = new_ka = new keep_array{n_null::val};
		new_link = new_ka->items_.items_;
	}
	any prev = link->h_->val_;
	ka->any_link_rebind(new_link);
	return true;
}
bool type_array::element_set(any & a_link, const any & key, keep_array * ka, wtext & msg) const {
	ref_var * link = ka->any_link_get();
	keep_array * inner_ka = link->h_->val_.value_.keep_->k_array();
	if( ref_var * new_link = inner_ka->set(key, n_null::val, msg) ) {
		any prev = link->h_->val_;
		ka->any_link_rebind(new_link);
		return true;
	}
	a_link = n_null::val;
	return false;
}
bool type_map::element_set(any & a_link, const any & key, keep_array * ka, wtext & msg) const {
	ref_var * link = ka->any_link_get();
	keep_map * inner_km = link->h_->val_.value_.keep_->k_map();
	if( ref_var * new_link = inner_km->set(key, n_null::val, msg) ) {
		any prev = link->h_->val_;
		ka->any_link_rebind(new_link);
		return true;
	}
	a_link = n_null::val;
	return false;
}

} // ns
} // ns

/*#ifdef __cplusplus
extern "C" {
#endif
__declspec(dllexport) bool __cdecl init_ksi_extension(const ksi::var::config * cfg) {
	ksi::var::hcfg = cfg;
	return true;
}
#ifdef __cplusplus
}
#endif*/
