#include "fn.inc.h"

namespace ksi {
namespace native {

inline void out(const var::any & v) {
	v.type_->out(v, std::wcout);
}

inline var::var_type get_type_value(const var::any & v) {
	var::var_type ret = v.type_;
	if( ret == &var::hcfg->t_type )
	ret = v.value_.type_;

	return ret;
}

// is

void fn_is_def(mod::fn_native_can_throw fne, space * spc, mod::fn_space * fns, t_stack * stk, base_log * log) {
	var::var_type tp = get_type_value(fns->args_[2]);
	fns->args_[0] = fns->args_[1].type_ == tp;
}

// is_not

void fn_is_not_def(mod::fn_native_can_throw fne, space * spc, mod::fn_space * fns, t_stack * stk, base_log * log) {
	var::var_type tp = get_type_value(fns->args_[2]);
	fns->args_[0] = fns->args_[1].type_ != tp;
}

// as

void fn_as_def(mod::fn_native_can_throw fne, space * spc, mod::fn_space * fns, t_stack * stk, base_log * log) {
	var::var_type tp = get_type_value(fns->args_[2]);
	wtext msg;
	fns->args_[0] = tp->convert_from(fns->args_[1], msg, spc);
	if( msg.h_->len_ )
	log->add({msg, fns->file_path(), fns->pos_});
}

// echo

void fn_echo_def(mod::fn_native_can_throw fne, space * spc, mod::fn_space * fns, t_stack * stk, base_log * log) {
	out(fns->args_[1]);
	fns->args_[0] = fns->args_[2];
}
void fn_echo_null(mod::fn_native_can_throw fne, space * spc, mod::fn_space * fns, t_stack * stk, base_log * log) {
	fns->args_[0] = fns->args_[2];
}
void init_sep(mod::fn_space * fns, var::any *& sep, var::any *& sep_pos) {
	sep = fns->args_ + 2;
	sep_pos = nullptr;
	if( id count; sep->type_ == &var::hcfg->t_array && (count = var::type_array::value_int(*sep)) ) {
		var::ref_var * rv = sep->value_.keep_->k_array()->items_.items_;
		if( count >= 2 )
		sep_pos = &rv[1].h_->val_;
		sep = &rv->h_->val_;
	}
}
void fn_echo_array(mod::fn_native_can_throw fne, space * spc, mod::fn_space * fns, t_stack * stk, base_log * log) {
	var::keep_array * ka = fns->args_[1].value_.keep_->k_array();
	if( ka->items_.count_ ) {
		var::any * sep, * sep_pos;
		init_sep(fns, sep, sep_pos);
		if( sep_pos ) {
			id pos = 0;
			for( var::ref_var & it : ka->items_ ) {
				if( pos )
				out(*sep);
				std::wcout << pos;
				out(*sep_pos);
				out(it.h_->val_);
				++pos;
			}
		} else {
			bool is_first = true;
			for( var::ref_var & it : ka->items_ ) {
				if( is_first )
				is_first = false;
				else
				out(*sep);
				out(it.h_->val_);
			}
		}
	}
	fns->args_[0] = fns->args_[2];
}
void fn_echo_map(mod::fn_native_can_throw fne, space * spc, mod::fn_space * fns, t_stack * stk, base_log * log) {
	var::keep_map * km = fns->args_[1].value_.keep_->k_map();
	if( km->ref_.h_->sorted_.count_ ) {
		var::any * sep, * sep_pos;
		init_sep(fns, sep, sep_pos);
		if( sep_pos ) {
			bool is_first = true;
			for( var::keep_map::t_items::cnode * it : km->ref_.h_->items_ ) {
				if( is_first )
				is_first = false;
				else
				out(*sep);
				out(it->key_);
				out(*sep_pos);
				out(it->val_.h_->val_);
			}
		} else {
			bool is_first = true;
			for( var::keep_map::t_items::cnode * it : km->ref_.h_->items_ ) {
				if( is_first )
				is_first = false;
				else
				out(*sep);
				out(it->val_.h_->val_);
			}
		}
	}
	fns->args_[0] = fns->args_[2];
}

// dump

void fn_dump_def(mod::fn_native_can_throw fne, space * spc, mod::fn_space * fns, t_stack * stk, base_log * log) {
	bool structured;
	if( fns->args_[2].type_ == &var::hcfg->t_bool )
	structured = fns->args_[2].value_.bool_;
	else {
		wtext msg;
		structured = fns->args_[2].type_->to_bool(fns->args_[2], msg);
	}
	fns->args_[1].type_->dump(fns->args_[1], std::wcout, structured);
	fns->args_[0] = fns->args_[1];
}

// text_size

void fn_text_size_text(mod::fn_native_can_throw fne, space * spc, mod::fn_space * fns, t_stack * stk, base_log * log) {
	fns->args_[0] = fns->args_[1].value_.keep_->k_text()->tx_.h_->len_;
}

// count

void fn_count_array(mod::fn_native_can_throw fne, space * spc, mod::fn_space * fns, t_stack * stk, base_log * log) {
	fns->args_[0] = var::type_array::value_int(fns->args_[1]);
}
void fn_count_map(mod::fn_native_can_throw fne, space * spc, mod::fn_space * fns, t_stack * stk, base_log * log) {
	fns->args_[0] = var::type_map::value_int(fns->args_[1]);
}
void fn_count_null(mod::fn_native_can_throw fne, space * spc, mod::fn_space * fns, t_stack * stk, base_log * log) {
	fns->args_[0] = 0LL;
}
void fn_count_def(mod::fn_native_can_throw fne, space * spc, mod::fn_space * fns, t_stack * stk, base_log * log) {
	fns->args_[0] = 1LL;
}

// type

void fn_type_def(mod::fn_native_can_throw fne, space * spc, mod::fn_space * fns, t_stack * stk, base_log * log) {
	fns->args_[0] = fns->args_[1].type_;
}

// clear

void fn_clear_array(mod::fn_native_can_throw fne, space * spc, mod::fn_space * fns, t_stack * stk, base_log * log) {
	var::keep_array * ka = fns->args_[1].value_.keep_->k_array();
	if( ka->lock_ )
	log->add({L"warning: #clear was called for the locked array.", fns->file_path(), fns->pos_});
	else
	ka->items_.clear();
	fns->args_[0] = fns->args_[1];
}

void fn_clear_map(mod::fn_native_can_throw fne, space * spc, mod::fn_space * fns, t_stack * stk, base_log * log) {
	var::keep_map * km = fns->args_[1].value_.keep_->k_map();
	if( km->lock_ )
	log->add({L"warning: #clear was called for the locked map.", fns->file_path(), fns->pos_});
	else
	km->clear();
	fns->args_[0] = fns->args_[1];
}

// sort

void fn_sort_array(mod::fn_native_can_throw fne, space * spc, mod::fn_space * fns, t_stack * stk, base_log * log) {
	var::keep_array * ka = fns->args_[1].value_.keep_->k_array();
	if( ka->lock_ )
	log->add({L"warning: #sort was called for the locked array.", fns->file_path(), fns->pos_});
	else if( id count = ka->items_.count_; count > 1 ) {
		ex::ref<id, true> pos = new id[count];
		for( id num = 0, * it = pos.h_, * end = pos.h_ + count; it < end; ++it, ++num )
		*it = num;
		t_cmp_array<var::ref_var *, var::cmp_any> cmp{ka->items_.items_, pos.h_};
		t_swp_array<var::ref_var *> swp{ka->items_.items_, pos.h_};
		t_sort::sort(ka->items_.items_, count, cmp, swp);
	}
	fns->args_[0] = fns->args_[1];
}

void fn_sort_map(mod::fn_native_can_throw fne, space * spc, mod::fn_space * fns, t_stack * stk, base_log * log) {
	var::keep_map * km = fns->args_[1].value_.keep_->k_map();
	if( km->lock_ )
	log->add({L"warning: #sort was called for the locked map.", fns->file_path(), fns->pos_});
	else if( id count = km->ref_.h_->sorted_.count_; count > 1 ) {
		ex::ref<t_map_pair, true> items = new t_map_pair[count];
		{
			id num = 0;
			t_map_pair * hmp = items.h_;
			for( t_map_pair::cnode * it : km->ref_.h_->items_ ) {
				hmp->node_ = it;
				hmp->pos_ = num;
				++num;
				++hmp;
			}
		}
		t_cmp_map<var::cmp_any> cmp;
		t_swp_map swp;
		t_sort::sort(items.h_, count, cmp, swp);
		// small hack
		ex::node * zero = &km->ref_.h_->items_.zero_;
		zero->next_ = zero;
		zero->prev_ = zero;
		for( t_map_pair * it = items.h_, * end = items.h_ + count; it < end; ++it )
		km->ref_.h_->items_.insert_after(it->node_, zero->prev_);
	}
	fns->args_[0] = fns->args_[1];
}

// key_sort

void fn_key_sort_array(mod::fn_native_can_throw fne, space * spc, mod::fn_space * fns, t_stack * stk, base_log * log) {
	fns->args_[0] = fns->args_[1];
}

void fn_key_sort_map(mod::fn_native_can_throw fne, space * spc, mod::fn_space * fns, t_stack * stk, base_log * log) {
	var::keep_map * km = fns->args_[1].value_.keep_->k_map();
	if( km->lock_ )
	log->add({L"warning: #key_sort was called for the locked map.", fns->file_path(), fns->pos_});
	else if( id count = km->ref_.h_->sorted_.count_; count > 1 ) {
		// small hack
		ex::node * zero = &km->ref_.h_->items_.zero_;
		zero->next_ = zero;
		zero->prev_ = zero;
		for( var::keep_map::t_items::cnode * it : km->ref_.h_->sorted_ )
		km->ref_.h_->items_.insert_after(it, zero->prev_);
	}
	fns->args_[0] = fns->args_[1];
}

// keys

void fn_keys_array(mod::fn_native_can_throw fne, space * spc, mod::fn_space * fns, t_stack * stk, base_log * log) {
	var::keep_array * ka = fns->args_[1].value_.keep_->k_array(), * keys;
	id count = ka->items_.count_;
	fns->args_[0] = keys = new var::keep_array(count ? count : 1);
	for( id i = 0; i < count; ++i )
	keys->items_.append_obj(i);
}
void fn_keys_map(mod::fn_native_can_throw fne, space * spc, mod::fn_space * fns, t_stack * stk, base_log * log) {
	var::keep_map * km = fns->args_[1].value_.keep_->k_map();
	var::keep_array * ka;
	fns->args_[0] = ka = new var::keep_array(km->ref_.h_->sorted_.count_);
	for( const var::keep_map::t_items::cnode * it : km->ref_.h_->items_ )
	ka->items_.append_obj(it->key_);
}

} // ns

namespace mod {

void native_wrap(fn_native_can_throw fne, space * spc, fn_space * fns, t_stack * stk, base_log * log) {
	if( std::exception_ptr eptr = fne(spc, fns, stk, log) )
	std::rethrow_exception(eptr);
}

std::exception_ptr call_func_from_native(
	base_func * fn, var::any & ret,
	space * spc, fn_space * caller, t_stack * stk,
	const also::t_pos & pos, base_log * log,
	const var::any & v1, const var::any & v2, const var::any & v3 = var::n_null::val
) {
	std::exception_ptr eptr;
	try {
		ret = (*fn)(spc, caller, stk, pos, log, v1, v2, v3);
	} catch( ... ) {
		eptr = std::current_exception();
	}
	return eptr;
}

wtext fn_space::file_path() {
	return prev_ && prev_->mod_ ? prev_->mod_->path_ : L"<none>";
}

wtext base_with_mod::mod_name() const {
	return mod_ && mod_->name_.type_ == &var::hcfg->t_text ? mod_->name_.value_.keep_->k_text()->tx_ : L"";
}

native_config * inst_native_config() {
	static native_config inst;
	return &inst;
}
native_config::native_config() {
	fn_null_ = fn_map_.obtain(L"#null");
	func_native * fun;
	// #is
	fun = fn_map_.obtain(L"#is");
	fun->over_.set_common(native::fn_is_def);
	// #is_not
	fun = fn_map_.obtain(L"#is_not");
	fun->over_.set_common(native::fn_is_not_def);
	// #as
	fun = fn_map_.obtain(L"#as");
	fun->over_.set_common(native::fn_as_def);
	// #clear
	fun = fn_map_.obtain(L"#clear");
	fun->over_.add(&var::hcfg->t_array, native::fn_clear_array);
	fun->over_.add(&var::hcfg->t_map, native::fn_clear_map);
	// #text_size
	fun = fn_map_.obtain(L"#text_size");
	fun->over_.add(&var::hcfg->t_text, native::fn_text_size_text);
	// #count
	fun = fn_map_.obtain(L"#count");
	fun->over_.set_common(native::fn_count_def);
	fun->over_.add(&var::hcfg->t_null, native::fn_count_null);
	fun->over_.add(&var::hcfg->t_array, native::fn_count_array);
	fun->over_.add(&var::hcfg->t_map, native::fn_count_map);
	// #echo
	fun = fn_map_.obtain(L"#echo");
	fun->over_.set_common(native::fn_echo_def);
	fun->over_.add(&var::hcfg->t_null, native::fn_echo_null);
	fun->over_.add(&var::hcfg->t_array, native::fn_echo_array);
	fun->over_.add(&var::hcfg->t_map, native::fn_echo_map);
	// #dump
	fun = fn_map_.obtain(L"#dump");
	fun->over_.set_common(native::fn_dump_def);
	// #sort
	fun = fn_map_.obtain(L"#sort");
	fun->over_.add(&var::hcfg->t_array, native::fn_sort_array);
	fun->over_.add(&var::hcfg->t_map, native::fn_sort_map);
	// #key_sort
	fun = fn_map_.obtain(L"#key_sort");
	fun->over_.add(&var::hcfg->t_array, native::fn_key_sort_array);
	fun->over_.add(&var::hcfg->t_map, native::fn_key_sort_map);
	// #keys
	fun = fn_map_.obtain(L"#keys");
	fun->over_.add(&var::hcfg->t_array, native::fn_keys_array);
	fun->over_.add(&var::hcfg->t_map, native::fn_keys_map);
	// #type
	fun = fn_map_.obtain(L"#type");
	fun->over_.set_common(native::fn_type_def);
}

} // ns
} // ns
