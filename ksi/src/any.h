#pragma once
#include "ksi_types.h"
#include "ksi_api.h"
#include <climits>
#include <cmath>

namespace ksi {

struct space;

namespace mod {

struct base_func;
struct native_config;
native_config * inst_native_config();
struct module;

} // ns

namespace var {

struct any;
struct ref_var;

// keep

struct keep_text;
struct keep_array;
struct keep_map;

struct base_keep : ex::with_deleter<base_keep> {
	ref_id refs_ = 1;

	virtual ~base_keep() = default;

	keep_text * k_text();
	keep_array * k_array();
	keep_map * k_map();
};

struct keep_text : public base_keep {
	wtext tx_;

	template <id N>
	keep_text(const Char (& cs)[N]) : tx_(cs) {}
	keep_text(const wtext & tx) : tx_(tx) {}
};

inline keep_text * base_keep::k_text() {
	return static_cast<keep_text *>(this);
}

struct with_lock {
	id lock_ = 0;

	inline void lock()		{ ++lock_; }
	inline void unlock()	{ --lock_; }
};

// each iterator

struct ei_base : public ex::with_deleter<ei_base> {
	virtual ~ei_base() = default;
	virtual bool valid() const { return false; }
	virtual void next() {}
	virtual void set_vars(ref_var & key, ref_var & val) {}
	virtual void set_vars_ref(ref_var & key, ref_var & val) {}

	template <class T>
	static ei_base * make(const any & v) { return new T(v); }
	using hfn_make = decltype(&ei_base::make<ei_base>);
};

// types

using map_keep = ex::def_map<
	base_keep *, bool, ex::map_del_plain, ex::map_del_plain, ex::cmp_std_plain,
	def_map_keep_r, def_map_keep_s
>;

struct base_type : ex::with_deleter<base_type> {
	const any * name_;
	id id_;
	bool is_meta_, is_ref_, is_struct_, is_native_, is_map_key_ = false;

	base_type(const base_type &) = delete;
	base_type & operator = (const base_type &) = delete;

	struct type_config {
		using array_types = ex::def_array<const base_type *, ex::del_plain, def_native_types_r, def_native_types_s>;
		using map_types = ex::def_map<
			wtext, const base_type *, ex::map_del_object, ex::map_del_plain, ex::cmp_std_plain,
			def_native_types_r, def_native_types_s
		>;
		array_types types_;
		map_types map_types_;

		static type_config * instance() {
			static type_config inst;
			return &inst;
		}
	};

	void native_set_name(const any * name, type_config * tc);

	base_type(type_config * tc, bool is_meta, bool is_ref, bool is_struct)
	: is_meta_(is_meta)
	, is_ref_(is_ref)
	, is_struct_(is_struct)
	, is_native_(true) {
		id_ = tc->types_.count_;
		tc->types_.append(this);
	}
	/*template <class Space>
	base_type(const any * name, Space * spc, bool is_meta, bool is_ref, bool is_struct)
	: name_(name)
	, is_meta_(is_meta)
	, is_ref_(is_ref)
	, is_struct_(is_struct)
	, is_native_(false) {
		id_ = spc->reg_type(this);
	}*/
	virtual ~base_type() = default;

	virtual id compare(const any & v1, const any & v2) const;
	virtual id cmp_null_x() const {
		return ex::cmp::less;
	}
	virtual id cmp_bool_x(bool v1, const any & v2) const;
	virtual id cmp_int_x(id v1, const any & v2) const;
	virtual id cmp_float_x(real v1, const any & v2) const;
	virtual id cmp_type_x(const base_type * v1, const any & v2) const;
	virtual id cmp_text_x(const wtext & v1, const any & v2) const;
	virtual id cmp_fn_x(const mod::base_func * v1, const any & v2) const;
	virtual id cmp_mod_x(const mod::module * v1, const any & v2) const;

	virtual void out(const any & v, std::wostream & wo) const {}
	virtual void dump(
		const any & v, std::wostream & wo,
		bool structured = false, id depth = 1, map_keep * hmk = nullptr
	) const {}

	virtual any convert_from(const any & a, wtext & msg, space * spc) const;
	virtual bool to_bool(const any & a, wtext & msg) const;
	virtual id to_int(const any & a, wtext & msg) const;
	virtual real to_float(const any & a, wtext & msg) const;
	virtual const base_type * to_type(const any & a, wtext & msg, space * spc) const;
	virtual any to_text(const any & a, wtext & msg) const;
	virtual const mod::base_func * to_fn(const any & a, wtext & msg, space * spc) const;
	virtual const mod::module * to_mod(const any & a, wtext & msg, space * spc) const;
	virtual any to_array(const any & a, wtext & msg) const;
	virtual any to_map(const any & a, wtext & msg) const;
	virtual any to_number(const any & a, wtext & msg) const;

	virtual any element_get(const any & a, const any & key) const;
	virtual bool element_set(any & a_link, const any & key, keep_array * ka, wtext & msg) const;

	virtual ei_base * get_iterator(id order, const any & v) const;
};

using var_type = const base_type *;

template <class Base, class Target>
struct with_convert_number : public Base {
	using Base::Base;

	id to_int(const any & a, wtext & msg) const override {
		return Target::value_int(a);
	}
	real to_float(const any & a, wtext & msg) const override {
		return Target::value_int(a);
	}
	any to_number(const any & a, wtext & msg) const override;
};

template <class Base, class Target>
struct with_convert_bool : public with_convert_number<Base, Target> {
	using with_convert_number<Base, Target>::with_convert_number;

	bool to_bool(const any & a, wtext & msg) const override {
		return Target::value_int(a);
	}
};

struct base_abstract : public base_type {
	base_abstract(type_config * tc)
	: base_type(tc, true, false, false) {}
};

struct type_any : public base_abstract {	// meta-type
	using base_abstract::base_abstract;

	any convert_from(const any & a, wtext & msg, space * spc) const override;
};

struct type_number : public base_abstract {	// meta-type
	using base_abstract::base_abstract;

	any convert_from(const any & a, wtext & msg, space * spc) const override;
};

struct base_simple : public base_type {
	base_simple(type_config * tc)
	: base_type(tc, false, false, false) {}
};

struct type_null : public base_simple {
	using base_simple::base_simple;

	id compare(const any & v1, const any & v2) const override;
	id cmp_null_x() const override {
		return ex::cmp::equal;
	}
	id cmp_bool_x(bool v1, const any & v2) const override {
		return ex::cmp::more;
	}

	void dump(
		const any & v, std::wostream & wo,
		bool structured = false, id depth = 1, map_keep * hmk = nullptr
	) const override;

	any to_array(const any & a, wtext & msg) const override;
	any to_map(const any & a, wtext & msg) const override;

	ei_base * get_iterator(id order, const any & v) const override {
		return new ei_base;
	}
};

struct type_bool : public base_simple {
	using base_simple::base_simple;

	id compare(const any & v1, const any & v2) const override;
	id cmp_bool_x(bool v1, const any & v2) const override;

	void out(const any & v, std::wostream & wo) const override;
	void dump(
		const any & v, std::wostream & wo,
		bool structured = false, id depth = 1, map_keep * hmk = nullptr
	) const override;

	any convert_from(const any & a, wtext & msg, space * spc) const override;
	bool to_bool(const any & a, wtext & msg) const override;
	id to_int(const any & a, wtext & msg) const override;
	real to_float(const any & a, wtext & msg) const override;
	any to_text(const any & a, wtext & msg) const override;
	any to_number(const any & a, wtext & msg) const override;
};

struct type_int : public with_convert_bool<base_simple, type_int> {
	using with_convert_bool<base_simple, type_int>::with_convert_bool;
	enum lim : id { min = LLONG_MIN, max = LLONG_MAX };
	static bool is_out_of_range(real n) {
		return n < min || n > max;
	}

	id compare(const any & v1, const any & v2) const override;
	id cmp_int_x(id v1, const any & v2) const override;
	id cmp_float_x(real v1, const any & v2) const override;

	void out(const any & v, std::wostream & wo) const override;
	void dump(
		const any & v, std::wostream & wo,
		bool structured = false, id depth = 1, map_keep * hmk = nullptr
	) const override;

	template <class T>
	static inline id value_int(const T & a) {
		return a.value_.int_;
	}
	any convert_from(const any & a, wtext & msg, space * spc) const override;
	var_type to_type(const any & a, wtext & msg, space * spc) const override;
	any to_text(const any & a, wtext & msg) const override;
	const mod::base_func * to_fn(const any & a, wtext & msg, space * spc) const override;
	const mod::module * to_mod(const any & a, wtext & msg, space * spc) const override;
};

struct type_float : public base_simple {
	using base_simple::base_simple;

	static double inf, inf_, nan;
	static bool is_nan(real v) {
		return v != v;
	}
	id compare(const any & v1, const any & v2) const override;
	id cmp_int_x(id v1, const any & v2) const override;
	id cmp_float_x(real v1, const any & v2) const override;

	void out(const any & v, std::wostream & wo) const override;
	void dump(
		const any & v, std::wostream & wo,
		bool structured = false, id depth = 1, map_keep * hmk = nullptr
	) const override;

	any convert_from(const any & a, wtext & msg, space * spc) const override;
	bool to_bool(const any & a, wtext & msg) const override;
	id to_int(const any & a, wtext & msg) const override;
	real to_float(const any & a, wtext & msg) const override;
	any to_text(const any & a, wtext & msg) const override;
	any to_number(const any & a, wtext & msg) const override;
};

struct type_type : public with_convert_number<base_simple, type_type> {
	using with_convert_number<base_simple, type_type>::with_convert_number;

	id compare(const any & v1, const any & v2) const override;
	id cmp_type_x(const base_type * v1, const any & v2) const override;

	void out(const any & v, std::wostream & wo) const override;
	void dump(
		const any & v, std::wostream & wo,
		bool structured = false, id depth = 1, map_keep * hmk = nullptr
	) const override;

	template <class T>
	static inline id value_int(const T & a) {
		return a.value_.type_->id_;
	}
	any convert_from(const any & a, wtext & msg, space * spc) const override;
	bool to_bool(const any & a, wtext & msg) const override;
	var_type to_type(const any & a, wtext & msg, space * spc) const override;
	any to_text(const any & a, wtext & msg) const override;
};

struct type_text : public base_type {
	type_text(type_config * tc) : base_type(tc, false, true, false) {}

	static wtext get_text(const any * h);
	static wtext get_text(const any & v);

	id compare(const any & v1, const any & v2) const override;
	id cmp_text_x(const wtext & v1, const any & v2) const override;

	void out(const any & v, std::wostream & wo) const override;
	void dump(
		const any & v, std::wostream & wo,
		bool structured = false, id depth = 1, map_keep * hmk = nullptr
	) const override;

	any convert_from(const any & a, wtext & msg, space * spc) const override;
	bool to_bool(const any & a, wtext & msg) const override;
	id to_int(const any & a, wtext & msg) const override;
	real to_float(const any & a, wtext & msg) const override;
	var_type to_type(const any & a, wtext & msg, space * spc) const override;
	any to_text(const any & a, wtext & msg) const override;
	const mod::base_func * to_fn(const any & a, wtext & msg, space * spc) const override;
	const mod::module * to_mod(const any & a, wtext & msg, space * spc) const override;
	any to_number(const any & a, wtext & msg) const override;
};

struct type_fn : public with_convert_number<base_simple, type_fn> {
	using with_convert_number<base_simple, type_fn>::with_convert_number;

	id compare(const any & v1, const any & v2) const override;
	id cmp_fn_x(const mod::base_func * v1, const any & v2) const override;

	void out(const any & v, std::wostream & wo) const override;
	void dump(
		const any & v, std::wostream & wo,
		bool structured = false, id depth = 1, map_keep * hmk = nullptr
	) const override;

	static id value_int(const any & a);
	any convert_from(const any & a, wtext & msg, space * spc) const override;
	bool to_bool(const any & a, wtext & msg) const override;
	any to_text(const any & a, wtext & msg) const override;
	const mod::base_func * to_fn(const any & a, wtext & msg, space * spc) const override;
};

struct type_module : public with_convert_bool<base_simple, type_fn> {
	using with_convert_bool<base_simple, type_fn>::with_convert_bool;

	id compare(const any & v1, const any & v2) const override;
	id cmp_mod_x(const mod::module * v1, const any & v2) const override;

	void out(const any & v, std::wostream & wo) const override;
	void dump(
		const any & v, std::wostream & wo,
		bool structured = false, id depth = 1, map_keep * hmk = nullptr
	) const override;

	static id value_int(const any & a);
	any convert_from(const any & a, wtext & msg, space * spc) const override;
	any to_text(const any & a, wtext & msg) const override;
	const mod::module * to_mod(const any & a, wtext & msg, space * spc) const override;
};

struct base_ref : public base_type {
	base_ref(type_config * tc, bool is_struct) : base_type(tc, false, true, is_struct) {}

	id compare(const any & v1, const any & v2) const override;
};

struct type_array : public with_convert_bool<base_ref, type_array> {
	using with_convert_bool<base_ref, type_array>::with_convert_bool;

	void out(const any & v, std::wostream & wo) const override;
	void inner_dump(const any & v, std::wostream & wo, bool structured, id depth, map_keep * hmk) const;
	void dump(
		const any & v, std::wostream & wo,
		bool structured = false, id depth = 1, map_keep * hmk = nullptr
	) const override;

	template <class T>
	static inline id value_int(const T & a) {
		return a.value_.keep_->k_array()->items_.count_;
	}
	any convert_from(const any & a, wtext & msg, space * spc) const override;
	any to_text(const any & a, wtext & msg) const override;
	any to_array(const any & a, wtext & msg) const override;
	any to_map(const any & a, wtext & msg) const override;

	any element_get(const any & a, const any & key) const override;
	bool element_set(any & a_link, const any & key, keep_array * ka, wtext & msg) const override;

	ei_base * get_iterator(id order, const any & v) const override;
};

struct type_map : public with_convert_bool<base_ref, type_map> {
	using with_convert_bool<base_ref, type_map>::with_convert_bool;

	void out(const any & v, std::wostream & wo) const override;
	void inner_dump(const any & v, std::wostream & wo, bool structured, id depth, map_keep * hmk) const;
	void dump(
		const any & v, std::wostream & wo,
		bool structured = false, id depth = 1, map_keep * hmk = nullptr
	) const override;

	template <class T>
	static inline id value_int(const T & a) {
		return a.value_.keep_->k_map()->ref_.h_->sorted_.count_;
	}
	any convert_from(const any & a, wtext & msg, space * spc) const override;
	any to_text(const any & a, wtext & msg) const override;
	any to_array(const any & a, wtext & msg) const override;
	any to_map(const any & a, wtext & msg) const override;

	any element_get(const any & a, const any & key) const override;
	bool element_set(any & a_link, const any & key, keep_array * ka, wtext & msg) const override;

	ei_base * get_iterator(id order, const any & v) const override;
};

// config

struct values_config;

struct config {
	base_type::type_config * tc;
	type_null		t_null;
	type_any		t_any;
	type_number		t_number;
	type_bool		t_bool;
	type_int		t_int;
	type_float		t_float;
	type_type		t_type;
	type_text		t_text;
	type_fn			t_fn;
	type_module		t_module;
	type_array		t_array;
	type_map		t_map;
	const values_config * values;
	mod::native_config * native_;
	const api * api_;
private:
	config(const api * v_api);
public:
	#ifdef KSI_LIB
	static void init(const api * v_api) {
		static const config inst(v_api);
	}
	#endif // KSI_LIB
};
extern const config * hcfg;
#ifndef KSI_LIB
void set_config(const config * h);
#endif // KSI_LIB

// any

struct ref_var;
enum class n_null { val };

struct any {
	union t_value {
		id			int_;
		bool		bool_;
		real		float_;
		var_type	type_;
		base_keep * keep_;
		const mod::base_func * func_;
		const mod::module * mod_;
	};
	t_value value_;
	const base_type * type_;

	void hold() const {
		if( type_->is_ref_ )
		value_.keep_->refs_ += 1;
	}
	void unhold() const {
		if( type_->is_ref_ ) {
			if( value_.keep_->refs_ <= 1 )
			value_.keep_->deleter_(value_.keep_);
			else
			value_.keep_->refs_ -= 1;
		}
	}

	any(const any & a) : value_(a.value_), type_(a.type_) {
		hold();
	}
	any & operator = (const any & a) {
		a.hold();
		unhold();
		value_ = a.value_;
		type_ = a.type_;
		return *this;
	}

	template <id N>
	any(const Char (& cs)[N]) : type_(&hcfg->t_text) {
		value_.keep_ = new keep_text(cs);
	}
	any(const wtext & tx) : type_(&hcfg->t_text) {
		value_.keep_ = new keep_text(tx);
	}

	any() : type_(&hcfg->t_null) {
		value_.int_ = 0;
	}
	any(n_null) : any() {}
	any(bool b) : type_(&hcfg->t_bool) {
		value_.bool_ = b;
	}
	any(id i) : type_(&hcfg->t_int) {
		value_.int_ = i;
	}
	any(real f) : type_(&hcfg->t_float) {
		value_.float_ = f;
	}
	any(var_type t) : type_(&hcfg->t_type) {
		value_.type_ = t;
	}
	any(const mod::base_func * fun) : type_(&hcfg->t_fn) {
		value_.func_ = fun;
	}
	any(keep_array * ka);
	any(keep_map * km);
	any(ref_var * link);

	~any() {
		unhold();
	}
};

template <class Base, class Target>
inline any with_convert_number<Base, Target>::to_number(const any & a, wtext & msg) const {
	return Target::value_int(a);
}

inline wtext type_text::get_text(const any * h) {
	return h->type_ == &hcfg->t_text ? h->value_.keep_->k_text()->tx_ : L"";
}
inline wtext type_text::get_text(const any & v) {
	return get_text(&v);
}

template <class T>
struct del_custom_type {
	using pass = T;

	static void del_one(T & item) {
		if( !item->is_native_ )
		item->deleter_(item);
	}

	template <class Hive>
	static void del_many(Hive * hive, id till) {
		for( T & item : hive->get_rev_iter(till) )
		del_one(item);
	}

	static void set_init(T * item, T val) {
		*item = val;
	}
};

struct values_config {
	any
	cn_null,
	cn_any,
	cn_number,
	cn_bool,
	cn_int,
	cn_float,
	cn_type,
	cn_text,
	cn_fn,
	cn_module,
	cn_array,
	cn_map,
	c_empty,
	c_zero,
	c_one;

	values_config() :
	cn_null		(L"null"),
	cn_any		(L"any"),
	cn_number	(L"number"),
	cn_bool		(L"bool"),
	cn_int		(L"int"),
	cn_float	(L"float"),
	cn_type		(L"type"),
	cn_text		(L"text"),
	cn_fn		(L"fn"),
	cn_module	(L"module"),
	cn_array	(L"array"),
	cn_map		(L"map"),
	c_empty		(L""),
	c_zero		(L"0"),
	c_one		(L"1")
	{}

	static const values_config * instance() {
		static const values_config inst;
		return &inst;
	}
};

struct any_r {
	any val_;
	ref_id refs_ = 1;
	void (* deleter_)(any_r *) = deleter;

	static void deleter(any_r * h) {
		delete h;
	}

	any_r(const any_r &) = delete;
	any_r & operator = (const any_r &) = delete;
};

struct ref_var {
	any_r * h_;

	void hold() const {
		h_->refs_ += 1;
	}
	void unhold() const {
		if( h_->refs_ <= 1 )
		h_->deleter_(h_);
		else
		h_->refs_ -= 1;
	}
	~ref_var() {
		unhold();
	}

	ref_var() {
		h_ = new any_r{};
	}
	ref_var(const any & a) {
		h_ = new any_r{a};
	}
	ref_var & operator = (const any & a) {
		h_->val_ = a;
		return *this;
	}
	//
	ref_var(const ref_var & rv) {
		h_ = new any_r{rv.h_->val_};
	}
	ref_var & operator = (const ref_var & rv) {
		h_->val_ = rv.h_->val_;
		return *this;
	}
	//
	ref_var(const ref_var * rv) {
		rv->hold();
		h_ = rv->h_;
	}
	ref_var & operator = (const ref_var * rv) {
		rv->hold();
		unhold();
		h_ = rv->h_;
		return *this;
	}

	inline void swap(ref_var & v) {
		ex::swap(h_, v.h_);
	}
	void reset() {
		unhold();
		h_ = new any_r{};
	}
};

struct cmp_any {
	static inline id compare(const any & a1, const any & a2) {
		return a1.type_->compare(a1, a2);
	}
	static inline id compare(const any * a1, const any * a2) {
		return a1->type_->compare(*a1, *a2);
	}
	static inline id compare(var_type t1, var_type t2) {
		return ex::cmp_std_plain::compare(t1->id_, t2->id_);
	}
	static inline id compare(ref_var * r1, ref_var * r2) {
		return compare(r1->h_->val_, r2->h_->val_);
	}
};

//
struct t_index {
	enum n_info { inside, out_less, out_more };
	n_info inf_;
	id pos_;
};

struct keep_array : public base_keep, public with_lock {
	using t_items = ex::array<ref_var, ex::del_object>;
	t_items items_;
	ref_var * link_ = nullptr;

	keep_array() : items_(def_array_r, def_array_s) {}
	keep_array(id reserve, id add = def_array_s) : items_(reserve + add, def_array_s) {}
	keep_array(std::initializer_list<any> lst) : items_(lst.size() + def_array_s, def_array_s) {
		for( const any & a : lst )
		items_.append_obj<const any &>(a);
	}
	keep_array(const keep_map * km);
	keep_array(ref_var * link) : with_lock{1}, items_(1, 1), /* lock_(1), */ link_(link) {
		items_.append_obj(link);
	}

	t_index pos_adopt(id pos) const {
		if( pos < 0 ) {
			pos = items_.count_ + pos;
			if( pos < 0 )
			return { t_index::out_less };

			return { t_index::inside, pos };
		}
		if( pos >= items_.count_ )
		return { t_index::out_more };

		return { t_index::inside, pos };
	}
	t_index pos_adopt(const any & key) const {
		if( key.type_ == &hcfg->t_bool )
		return { key.value_.bool_ ? t_index::out_more : t_index::out_less };

		if( key.type_ != &hcfg->t_int )
		return { t_index::out_more };

		return pos_adopt(key.value_.int_);
	}
	any get(const any & key) const {
		if( key.type_ != &hcfg->t_int )
		return n_null::val;

		if( t_index index = pos_adopt(key.value_.int_); index.inf_ == t_index::inside )
		return items_.items_[index.pos_].h_->val_;

		return n_null::val;
	}
	ref_var * set(const any & key, const any & val, wtext & msg) {
		if( link_ )
		return nullptr;

		ref_var * ret = nullptr;
		t_index index = pos_adopt(key);
		switch( index.inf_ ) {
		case t_index::inside :
			items_.items_[index.pos_] = val;
			ret = items_.items_ + index.pos_;
			break;
		case t_index::out_more :
			if( lock_ )
			msg = L"warning: Unable to append value to locked array.";
			else {
				items_.append_obj(val);
				ret = &items_.last(0);
			}
			break;
		case t_index::out_less :
			if( lock_ )
			msg = L"warning: Unable to prepend value to locked array.";
			else {
				items_.insert_obj(0, val);
				ret = items_.items_;
			}
			break;
		}
		return ret;
	}
	void unset(const any & key) {
		if( t_index index = pos_adopt(key.value_.int_); index.inf_ == t_index::inside )
		items_.remove(index.pos_);
	}

	static keep_array * any_link_check(const any & v) {
		keep_array * ka;
		return v.type_ == &hcfg->t_array && ( ka = v.value_.keep_->k_array() )->link_ ? ka : nullptr;
	}
	ref_var * any_link_get() {
		return items_.items_;
	}
	ref_var * any_link_src() {
		return link_;
	}
	void any_link_rebind(ref_var * link) {
		link_ = link;
		*items_.items_ = link;
	}
};

inline keep_array * base_keep::k_array() {
	return static_cast<keep_array *>(this);
}

//
struct keep_map : public base_keep, public with_lock {
	using t_items = ex::map<any, ref_var, ex::map_del_object, ex::map_del_object, cmp_any>;
	using t_ref = ex::ref<t_items, false>;

	t_ref ref_;
	id next_id_ = 0;
	bool is_full_id_ = false;

	struct pair {
		any key_, val_;
	};

	void clear() {
		ref_.h_->clear();
		next_id_ = 0;
		is_full_id_ = false;
	}
	any get(const any & key) const {
		if( !key.type_->is_map_key_ )
		return n_null::val;

		t_items * items = ref_.h_;
		if( t_items::t_res_node res = items->find_node(key) )
		return res.pos_->val_.h_->val_;

		return n_null::val;
	}
	ref_var * set(const any & key, const any & val, wtext & msg, bool reorder = false) {
		ref_var * ret = nullptr;
		if( key.type_->is_map_key_ ) {
			if( key.type_ == &hcfg->t_int && key.value_.int_ >= next_id_ ) {
				if( id i = key.value_.int_; i == type_int::max ) {
					is_full_id_ = true;
					next_id_ = i;
				} else
				next_id_ = key.value_.int_ +1;
			}
			t_items * items = ref_.h_;
			ex::id_search_res res = items->find_key(key);
			if( res ) {
				ex::same_key sk;
				if( reorder && lock_ ) {
					msg = L"notice: Unable to reorder element of locked map, so value is just updated.";
					sk = ex::same_key::update;
				} else
				sk = reorder ? ex::same_key::reorder : ex::same_key::update;

				ret = &(items->v_inner_insert_after<const any &>(key, val, sk, items->in_end(), res)->val_);
			} else {
				if( lock_ )
				msg = L"warning: Unable to add value to locked map.";
				else
				ret = &(items->v_inner_insert_after<const any &>(
					key, val,
					reorder ? ex::same_key::reorder : ex::same_key::update,
					items->in_end(), res
				)->val_);
			}
		} else {
			if( lock_ )
			msg = L"warning: Unable to add value to locked map.";
			else if( is_full_id_ )
			msg = L"warning: Max auto-increment value is already reached when adding to $map, so value was not added.";
			else {
				t_items * items = ref_.h_;
				ex::node * after = (key.type_ == &hcfg->t_bool && !key.value_.bool_) ? items->in_begin() : items->in_end();
				ret = &(items->v_insert_after<const any &>(
					next_id_, val,
					reorder ? ex::same_key::reorder : ex::same_key::update,
					after
				)->val_);
				if( next_id_ == type_int::max )
				is_full_id_ = true;
				else
				next_id_ += 1;
			}
		}
		return ret;
	}
	void unset(const any & key) {
		ref_.h_->detach(key);
	}

	keep_map() : ref_(new t_items(def_map_r, def_map_s) ) {}
	keep_map(id reserve, id add = def_map_s) : ref_(new t_items(reserve + add, def_map_s) ) {}
	keep_map(std::initializer_list<pair> lst) : ref_(new t_items(lst.size() + def_map_s, def_array_s) ) {
		wtext msg;
		for( const pair & it : lst )
		set(it.key_, it.val_, msg, true);
	}
	keep_map(const keep_array * ka) : ref_(new t_items(ka->items_.count_ + def_map_s, def_array_s) ) {
		wtext msg;
		for( const ref_var & it : ka->items_ )
		set(n_null::val, it.h_->val_, msg, false);
	}
};

inline keep_map * base_keep::k_map() {
	return static_cast<keep_map *>(this);
}

// each iterator simple

struct ei_simple : public ei_base {
	any val_;
	bool valid_ = true;

	ei_simple(const any & v) : val_(v) {}
	bool valid() const override {
		return valid_;
	}
	void next() override {
		valid_ = false;
		val_ = n_null::val;
	}
	inline void impl_set_vars(ref_var & key, ref_var & val) {
		key.h_->val_ = n_null::val;
		val.h_->val_ = val_;
	}
	void set_vars(		ref_var & key, ref_var & val) { impl_set_vars(key, val); }
	void set_vars_ref(	ref_var & key, ref_var & val) { impl_set_vars(key, val); }
};

// each iterator array

struct ei_array_base : public ei_base {
	any val_;
	ref_var * cur_, * end_;
	id pos_;

	ei_array_base(const any & v) : val_(v) {}
	~ei_array_base() {
		val_.value_.keep_->k_array()->unlock();
	}

	bool valid() const override {
		return cur_ != end_;
	}
	void set_vars(ref_var & key, ref_var & val) override {
		key.h_->val_ = pos_;
		val = *cur_;
	}
	void set_vars_ref(ref_var & key, ref_var & val) override {
		key.h_->val_ = pos_;
		val = cur_;
	}
};

struct ei_array_asc : public ei_array_base {
	ei_array_asc(const any & v) : ei_array_base(v) {
		keep_array * ka = val_.value_.keep_->k_array();
		ka->lock();
		cur_ = ka->items_.begin();
		end_ = ka->items_.end();
		pos_ = 0;
	}

	void next() override {
		++cur_;
		++pos_;
	}
};

struct ei_array_desc : public ei_array_base {
	ei_array_desc(const any & v) : ei_array_base(v) {
		keep_array * ka = val_.value_.keep_->k_array();
		ka->lock();
		cur_ = ka->items_.rbegin();
		end_ = ka->items_.rend();
		pos_ = ka->items_.count_ -1;
	}

	void next() override {
		--cur_;
		--pos_;
	}
};

// each iterator map

struct ei_map_base : public ei_base {
	using cnode = typename keep_map::t_items::cnode;

	any val_;

	ei_map_base(const any & v, keep_map * km) : val_(v) {
		km->lock();
	}
	~ei_map_base() {
		val_.value_.keep_->k_map()->unlock();
	}
};

template <bool IsRev>
struct ei_map_seq : public ei_map_base {
	using t_iter = ex::list_iter<IsRev, cnode>;

	static t_iter get_iter(keep_map * km) {
		keep_map::t_items * m = km->ref_.h_;
		if constexpr( IsRev )
		return m->items_.get_rev_iter();
		else
		return m->items_.begin();
	}

	t_iter iter_;

	ei_map_seq(const any & v) : ei_map_seq(v, v.value_.keep_->k_map() ) {}
	ei_map_seq(const any & v, keep_map * km) : ei_map_base(v, km), iter_( get_iter(km) ) {}

	bool valid() const override {
		return iter_.cur_ != iter_.end_;
	}
	void next() override {
		++iter_;
	}
	void set_vars(ref_var & key, ref_var & val) override {
		cnode * nd = *iter_;
		key = nd->key_;
		val = nd->val_;
	}
	void set_vars_ref(ref_var & key, ref_var & val) override {
		cnode * nd = *iter_;
		key = nd->key_;
		val = &nd->val_;
	}
};

template <bool IsRev>
struct ei_map_key : public ei_map_base {
	cnode ** cur_, ** end_;

	ei_map_key(const any & v) : ei_map_key(v, v.value_.keep_->k_map() ) {}
	ei_map_key(const any & v, keep_map * km) : ei_map_base(v, km) {
		keep_map::t_items * m = km->ref_.h_;
		if constexpr( IsRev ) {
			cur_ = m->sorted_.rbegin();
			end_ = m->sorted_.rend();
		} else {
			cur_ = m->sorted_.begin();
			end_ = m->sorted_.end();
		}
	}

	bool valid() const override {
		return cur_ != end_;
	}
	void next() override {
		if constexpr( IsRev )
		--cur_;
		else
		++cur_;
	}
	void set_vars(ref_var & key, ref_var & val) override {
		key = (*cur_)->key_;
		val = (*cur_)->val_;
	}
	void set_vars_ref(ref_var & key, ref_var & val) override {
		key = (*cur_)->key_;
		val = &(*cur_)->val_;
	}
};

//
template <class Op>
struct number_op {
	template <class T1>
	static any inner_calc(T1 n1, const any & a2) {
		if( a2.type_ == &hcfg->t_int ) {
			id n2 = a2.value_.int_;
			return Op::calc(n1, n2);
		} else if( a2.type_ == &hcfg->t_float ) {
			real n2 = a2.value_.float_;
			return Op::calc(n1, n2);
		}
		return n_null::val;
	}
	static any calc(const any & v1, const any & v2, wtext & msg1, wtext & msg2) {
		any a1 = v1.type_->to_number(v1, msg1);
		any a2 = v2.type_->to_number(v2, msg2);
		if( a1.type_ == &hcfg->t_int ) {
			id n1 = a1.value_.int_;
			return inner_calc(n1, a2);
		} else if( a1.type_ == &hcfg->t_float ) {
			real n1 = a1.value_.float_;
			return inner_calc(n1, a2);
		}
		return n_null::val;
	}
};
struct op_plus {
	static wtext get_name() { return L"addition"; }
	static any calc(id n1, id n2) {
		real ret = static_cast<real>(n1) + n2;
		if( type_int::is_out_of_range(ret) )
		return ret;
		return n1 + n2;
	}
	template <class T1, class T2>
	static any calc(T1 n1, T2 n2) {
		return n1 + n2;
	}
};
struct op_minus {
	static wtext get_name() { return L"subtraction"; }
	static any calc(id n1, id n2) {
		real ret = static_cast<real>(n1) - n2;
		if( type_int::is_out_of_range(ret) )
		return ret;
		return n1 - n2;
	}
	template <class T1, class T2>
	static any calc(T1 n1, T2 n2) {
		return n1 - n2;
	}
};
struct op_mult {
	static wtext get_name() { return L"multiplication"; }
	static any calc(id n1, id n2) {
		real ret = static_cast<real>(n1) * n2;
		if( type_int::is_out_of_range(ret) )
		return ret;
		return n1 * n2;
	}
	template <class T1, class T2>
	static any calc(T1 n1, T2 n2) {
		return n1 * n2;
	}
};
struct op_div {
	static wtext get_name() { return L"division"; }
	static any calc(id n1, id n2) {
		if( n2 == 1 )
		return n1;
		if( n2 == n1 )
		return 1LL;
		if( n2 == -1 && n1 > type_int::min )
		return -n1;
		return static_cast<real>(n1) / n2;
	}
	template <class T1, class T2>
	static any calc(T1 n1, T2 n2) {
		return n1 / n2;
	}
};
struct op_mod {
	static wtext get_name() { return L"modulo"; }
	static any calc(id n1, id n2) {
		real ret = std::fmod(n1, n2);
		if( n2 == 0 || type_int::is_out_of_range(ret) )
		return ret;
		return n1 % n2;
	}
	template <class T1, class T2>
	static any calc(T1 n1, T2 n2) {
		return std::fmod(n1, n2);
	}
};

} // ns
} // ns

inline std::wostream & operator << (std::wostream & wo, const ksi::var::any & a) {
	a.type_->out(a, wo);
	return wo;
}
