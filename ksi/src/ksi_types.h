#pragma once
#include "extra.h"

namespace ksi {

using ex::id;
using ex::ref_id;
using ex::uid;
using ex::real;
using ex::wtext;
using Char = wtext::Char;

constexpr Char endl = L'\n';

// array/map ~ reserve and step
struct def {
	enum n_def : id {
		native_types_r = 20, native_types_s = 8,
		log_r = 8, log_s = 4,
		modules_r = 4, modules_s = 4,
		types_r = 32, types_s = 8,
		types_mod_r = 5, types_mod_s = 5,
		array_r = 5, array_s = 5,
		map_r = 5, map_s = 5,
		over_r = 4, over_s = 4,
		funcs_r = 8, funcs_s = 4,
		var_names_r = 6, var_names_s = 4,
		stack_r = 10, stack_s = 5,
		tokens_r = 16, tokens_s = 16,
		map_keep_r = 4, map_keep_s = 4,
		instr_r = 8, instr_s = 8,
		sides_r = 2, sides_s = 4,
		literals_r = 10, literals_s = 8,
		ast_nodes_r = 4, ast_nodes_s = 4,
		ast_trees_r = 4, ast_trees_s = 4,
		ast_scopes_r = 1, ast_scopes_s = 2,
		ast_pos_r = 2, ast_pos_s = 4,
		for_implode_r = 2, for_implode_s = 4,
		each_iterators_r = 2, each_iterators_s = 4,
		ast_fn_info_r = 8, ast_fn_info_s = 8,
		ast_fn_call_r = 8, ast_fn_call_s = 8,
		type_static_hive_r = 4, type_static_hive_s = 4
	};
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
	<< msg.file_ << ksi::endl << msg.msg_ << ksi::endl;
}

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
	using t_log = ex::def_array<also::message, ex::del_object, def::log_r, def::log_s>;
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

struct request_info {
	wtext
	url_,
	scheme_,
	host_,
	path_,
	query_,
	hash_,
	method_,
	protocol_;
};

struct run_args {
	bool
	debug_		= false,
	show_log_	= false;
	request_info * req_inf_ = nullptr;

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
