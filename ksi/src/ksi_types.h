#pragma once
#include "extra.h"

namespace ksi {

//using namespace ex;
using ex::id;
using ex::ref_id;
using ex::uid;
using ex::real;
using ex::wtext;
using Char = wtext::Char;

// array reserve and step
enum def : id {
	def_native_types_r = 20, def_native_types_s = 8,
	def_log_r = 8, def_log_s = 4,
	def_modules_r = 4, def_modules_s = 4,
	def_types_r = 32, def_types_s = 8,
	def_array_r = 5, def_array_s = 5,
	def_map_r = 5, def_map_s = 5,
	def_over_r = 4, def_over_s = 4,
	def_funcs_r = 8, def_funcs_s = 4,
	def_var_names_r = 6, def_var_names_s = 4,
	def_stack_r = 10, def_stack_s = 5,
	def_tokens_r = 16, def_tokens_s = 16,
	def_map_keep_r = 4, def_map_keep_s = 4,
	def_instr_r = 8, def_instr_s = 8,
	def_sides_r = 2, def_sides_s = 4,
	def_literals_r = 10, def_literals_s = 8,
	def_ast_nodes_r = 4, def_ast_nodes_s = 4,
	def_ast_trees_r = 4, def_ast_trees_s = 4,
	def_ast_scopes_r = 1, def_ast_scopes_s = 2,
	def_ast_pos_r = 2, def_ast_pos_s = 4,
	def_for_implode_r = 2, def_for_implode_s = 4,
	def_each_iterators_r = 2, def_each_iterators_s = 4,
	def_ast_fn_info_r = 8, def_ast_fn_info_s = 8,
	def_ast_fn_call_r = 8, def_ast_fn_call_s = 8
};

namespace also {

struct t_pos {
	id line_, col_;
};

struct message {
	wtext msg_, file_;
	t_pos pos_;
};

inline std::wostream & operator << (std::wostream & wo, const message & msg) {
	return wo << L"line " << msg.pos_.line_ << L", col " << msg.pos_.col_ << L": "
	<< msg.file_ << std::endl << msg.msg_ << std::endl;
}

template <class T, template <class Item> class Closer, id Reserve, id Step>
struct hive {
	using t_arr = ex::def_array<T, Closer, Reserve, Step>;
	using t_map = ex::def_map<
		wtext, id, ex::map_del_object, ex::map_del_plain, ex::cmp_std_plain,
		Reserve, Step
	>;
	using pass = typename t_arr::pass;
	using t_res = ex::search_res<T>;
	t_arr arr_;
	t_map map_;

	id inner_add(const wtext & name, pass item, ex::id_search_res res) {
		if( res )
		return map_.sorted_.items_[res.pos_]->val_;
		id ret = arr_.count_;
		arr_.append(item);
		map_.inner_insert_after(name, ret, ex::same_key::ignore, map_.in_end(), res);
		return ret;
	}
	inline id add(const wtext & name, pass item) {
		return inner_add(name, item, map_.find_key(name));
	}
	ex::id_search_res find_pos(const wtext & name) {
		if( ex::id_search_res res = map_.find_key(name) )
		return {true, map_.sorted_.items_[res.pos_]->val_};
		else
		return {false};
	}
	t_res find_item(const wtext & name) {
		if( ex::id_search_res res = find_pos(name) )
		return {true, arr_.items_[res.pos_]};
		else
		return {false};
	}
};

} // ns

// log

struct base_log {
	bool filled_ = false;
	virtual void add(const also::message & msg) {}
	virtual void out(std::wostream & wo) const {}
};

struct log_last_only : public base_log {
	also::message msg_;
	void add(const also::message & msg) override {
		filled_ = true;
		msg_ = msg;
	}
	void out(std::wostream & wo) const override {
		wo << msg_;
	}
};

struct log_array : public base_log {
	using t_log = ex::def_array<also::message, ex::del_object, def_log_r, def_log_s>;
	t_log log_;
	void add(const also::message & msg) override {
		filled_ = true;
		log_.append(msg);
	}
	void out(std::wostream & wo) const override {
		for( const also::message & msg : log_ )
		wo << msg;
	}
};

struct log_file : public base_log {
	wtext path_;
	std::char_traits<wchar_t>::pos_type pos_;
	log_file(const wtext & path);
	void add(const also::message & msg) override;
	void out(std::wostream & wo) const override;
};

//

struct run_args {
	bool
	debug_		= false,
	show_log_	= false;

	void init(wchar_t ** args, int args_count) {
		if( args_count > 2 ) {
			for( id n = 2; n < args_count; ++n ) {
				if( !debug_ && !std::wcscmp(args[n], L"-debug") )
				debug_ = true;
				else if( !show_log_ && !std::wcscmp(args[n], L"-show_log") )
				show_log_ = true;
			}
		}
	}
};

namespace var {

enum n_each_order : id { order_asc, order_desc, order_key_asc, order_key_desc };

} // ns
} // ns
