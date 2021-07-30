#pragma once
#include "tokens.h"

namespace ksi {
namespace mod {

struct instr_type;

} // ns
namespace ast {

struct op_info_lite;

} // ns
namespace tokens {

struct token_set_module_name : public base_token {
	ex::wtext name_;

	token_set_module_name(const ex::wtext & name) : name_(name) {}
	wtext get_name() const override { return L"token_set_module_name"; }
	void prepare(space * spc, ast::prepare_data * pd, base_log * log) override;
};

// fn

struct token_plain : public base_token {
	also::t_pos pos_;

	token_plain(const also::t_pos & pos) : pos_(pos) {}
	wtext get_name() const override { return L"token_plain"; }
	void perform(space * spc, ast::prepare_data * pd, base_log * log) override;
};

struct token_fn_begin : public base_token {
	also::t_pos pos_;
	wtext fn_name_;

	token_fn_begin(const also::t_pos & pos, const wtext & fn_name) : pos_(pos), fn_name_(fn_name) {}
	wtext get_name() const override { return L"token_begin_fn"; }
	void perform(space * spc, ast::prepare_data * pd, base_log * log) override;
};

struct token_fn_end : public base_token {
	wtext get_name() const override { return L"token_end_fn"; }
	void perform(space * spc, ast::prepare_data * pd, base_log * log) override;
};

struct token_fn_set_overload : public base_token {
	also::t_pos pos_;
	wtext type_name_;
	bool is_static_;

	token_fn_set_overload(
		const also::t_pos & pos, const wtext & type_name, bool is_static
	) : pos_(pos), type_name_(type_name), is_static_(is_static) {}
	wtext get_name() const override { return L"token_set_fn_overload"; }
	void perform(space * spc, ast::prepare_data * pd, base_log * log) override;
};

struct base_token_fn_add_arg {
	also::t_pos pos_;
	wtext arg_name_;

	base_token_fn_add_arg(const also::t_pos & pos, const wtext & arg_name) : pos_(pos), arg_name_(arg_name) {}
	void perform_1(space * spc, ast::prepare_data * pd, base_log * log);
	void perform_2(space * spc, ast::prepare_data * pd, base_log * log);
};

template <bool Is_first>
struct token_fn_add_arg :
public base_token,
public base_token_fn_add_arg {
	using base_token_fn_add_arg::base_token_fn_add_arg;

	wtext get_name() const override {
		if constexpr( Is_first )
		return L"token_fn_add_var_1";
		else
		return L"token_fn_add_var_2";
	}
	void perform(space * spc, ast::prepare_data * pd, base_log * log) override {
		if constexpr( Is_first )
		perform_1(spc, pd, log);
		else
		perform_2(spc, pd, log);
	}
};

// literals

struct token_put_null : public base_token {
	also::t_pos pos_;

	token_put_null(also::t_pos pos) : pos_(pos) {}
	wtext get_name() const override { return L"token_put_null"; }
	void perform(space * spc, ast::prepare_data * pd, base_log * log) override;
};

struct token_put_bool : public base_token {
	also::t_pos pos_;
	bool val_;

	token_put_bool(also::t_pos pos, bool val) : pos_(pos), val_(val) {}
	wtext get_name() const override { return L"token_put_bool"; }
	void perform(space * spc, ast::prepare_data * pd, base_log * log) override;
};

struct token_put_int : public base_token {
	also::t_pos pos_;
	id val_;

	token_put_int(also::t_pos pos, id val) : pos_(pos), val_(val) {}
	wtext get_name() const override { return L"token_put_int"; }
	void perform(space * spc, ast::prepare_data * pd, base_log * log) override;
};

struct token_put_float : public base_token {
	also::t_pos pos_;
	real val_;

	token_put_float(also::t_pos pos, real val) : pos_(pos), val_(val) {}
	wtext get_name() const override { return L"token_put_float"; }
	void perform(space * spc, ast::prepare_data * pd, base_log * log) override;
};

struct token_put_text : public base_token {
	also::t_pos pos_;
	wtext val_;

	token_put_text(also::t_pos pos, const wtext & val) : pos_(pos), val_(val) {}
	wtext get_name() const override { return L"token_put_text"; }
	void perform(space * spc, ast::prepare_data * pd, base_log * log) override;
};

struct token_put_type : public base_token {
	also::t_pos pos_;
	wtext type_name_;

	token_put_type(also::t_pos pos, const wtext & type_name) : pos_(pos), type_name_(type_name) {}
	wtext get_name() const override { return L"token_put_type"; }
	void perform(space * spc, ast::prepare_data * pd, base_log * log) override;
};

// var

struct token_put_var : public base_token {
	also::t_pos pos_;
	wtext var_name_;

	token_put_var(also::t_pos pos, const wtext & var_name) : pos_(pos), var_name_(var_name) {}
	wtext get_name() const override { return L"token_put_var"; }
	void perform(space * spc, ast::prepare_data * pd, base_log * log) override;
};

struct token_put_var_link : public base_token {
	also::t_pos pos_;
	wtext var_name_;

	token_put_var_link(also::t_pos pos, const wtext & var_name) : pos_(pos), var_name_(var_name) {}
	wtext get_name() const override { return L"token_put_var_link"; }
	void perform(space * spc, ast::prepare_data * pd, base_log * log) override;
};

// scope

struct token_scope_begin : public base_token {
	also::t_pos pos_;

	token_scope_begin(also::t_pos pos) : pos_(pos) {}
	wtext get_name() const override { return L"token_begin_scope"; }
	void perform(space * spc, ast::prepare_data * pd, base_log * log) override;
};

struct token_scope_end : public base_token {
	wtext get_name() const override { return L"token_end_scope"; }
	void perform(space * spc, ast::prepare_data * pd, base_log * log) override;
};

// array

struct token_array_begin : public base_token {
	also::t_pos pos_;

	token_array_begin(also::t_pos pos) : pos_(pos) {}
	wtext get_name() const override { return L"token_begin_array"; }
	void perform(space * spc, ast::prepare_data * pd, base_log * log) override;
};

struct token_array_end : public token_scope_end {
	wtext get_name() const override { return L"token_end_array"; }
};

// map

struct token_map_begin : public base_token {
	also::t_pos pos_;

	token_map_begin(also::t_pos pos) : pos_(pos) {}
	wtext get_name() const override { return L"token_begin_map"; }
	void perform(space * spc, ast::prepare_data * pd, base_log * log) override;
};

struct token_map_end : public token_scope_end {
	wtext get_name() const override { return L"token_end_map"; }
};

// subscript

struct token_bracket_begin : public base_token {
	also::t_pos pos_;
	const ast::op_info_lite * op_;

	token_bracket_begin(also::t_pos pos, const ast::op_info_lite * op) : pos_(pos), op_(op) {}
	wtext get_name() const override { return L"token_bracket_begin"; }
	void perform(space * spc, ast::prepare_data * pd, base_log * log) override;
};

struct token_bracket_end : public token_scope_end {
	wtext get_name() const override { return L"token_end_bracket"; }
};

// condition ? action | else_action ;

struct token_then : public base_token {
	also::t_pos pos_;
	const ast::op_info_lite * op_;

	token_then(also::t_pos pos, const ast::op_info_lite * op) : pos_(pos), op_(op) {}
	wtext get_name() const override { return L"token_then"; }
	void perform(space * spc, ast::prepare_data * pd, base_log * log) override;
};

struct token_else : public base_token {
	wtext get_name() const override { return L"token_else"; }
	void perform(space * spc, ast::prepare_data * pd, base_log * log) override;
};

struct token_condition_end : public base_token {
	id depth_;

	token_condition_end(id depth) : depth_(depth) {}
	wtext get_name() const override { return L"token_end_condition"; }
	void perform(space * spc, ast::prepare_data * pd, base_log * log) override;
};

// while

struct token_loop_while_begin : public base_token {
	also::t_pos pos_;
	const mod::instr_type * i_type_;

	token_loop_while_begin(also::t_pos pos, const mod::instr_type * i_type) : pos_(pos), i_type_(i_type) {}
	wtext get_name() const override { return L"token_begin_loop_while"; }
	void perform(space * spc, ast::prepare_data * pd, base_log * log) override;
};

struct token_loop_while_end : public token_scope_end {
	wtext get_name() const override { return L"token_end_loop_while"; }
};

struct token_loop_while_also_block : public base_token {
	wtext get_name() const override { return L"token_loop_while_also_block"; }
	void perform(space * spc, ast::prepare_data * pd, base_log * log) override;
};

struct token_loop_while_body : public base_token {
	wtext get_name() const override { return L"token_loop_while_body"; }
	void perform(space * spc, ast::prepare_data * pd, base_log * log) override;
};

struct token_loop_while_body_rest : public base_token {
	wtext get_name() const override { return L"token_loop_while_body_rest"; }
	void perform(space * spc, ast::prepare_data * pd, base_log * log) override;
};

// each

struct token_loop_each_begin : public base_token {
	also::t_pos pos_;

	token_loop_each_begin(also::t_pos pos) : pos_(pos) {}
	wtext get_name() const override { return L"token_begin_loop_each"; }
	void perform(space * spc, ast::prepare_data * pd, base_log * log) override;
};

struct token_loop_each_end : public token_scope_end {
	wtext get_name() const override { return L"token_end_loop_each"; }
};

struct token_loop_each_order : public base_token {
	id order_;

	token_loop_each_order(id order) : order_(order) {}
	wtext get_name() const override { return L"token_loop_each_set_order"; }
	void perform(space * spc, ast::prepare_data * pd, base_log * log) override;
};

struct token_loop_each_key : public base_token {
	also::t_pos pos_;
	wtext var_name_;

	token_loop_each_key(also::t_pos pos, const wtext & var_name) : pos_(pos), var_name_(var_name) {}
	wtext get_name() const override { return L"token_loop_each_key"; }
	void perform(space * spc, ast::prepare_data * pd, base_log * log) override;
};

struct token_loop_each_val : public base_token {
	wtext var_name_;
	bool is_by_ref_;

	token_loop_each_val(const wtext & var_name, bool is_by_ref) : var_name_(var_name), is_by_ref_(is_by_ref) {}
	wtext get_name() const override { return L"token_loop_each_val"; }
	void perform(space * spc, ast::prepare_data * pd, base_log * log) override;
};

struct token_loop_each_also_block : public base_token {
	wtext get_name() const override { return L"token_loop_each_also_block"; }
	void perform(space * spc, ast::prepare_data * pd, base_log * log) override;
};

struct token_loop_each_body : public base_token {
	bool was_also_block_;

	token_loop_each_body(bool was_also_block) : was_also_block_(was_also_block) {}
	wtext get_name() const override { return L"token_loop_each_body"; }
	void perform(space * spc, ast::prepare_data * pd, base_log * log) override;
};

struct token_loop_each_body_rest : public base_token {
	wtext get_name() const override { return L"token_loop_each_body_rest"; }
	void perform(space * spc, ast::prepare_data * pd, base_log * log) override;
};

// for

struct token_loop_for_begin : public token_loop_each_begin {
	using token_loop_each_begin::token_loop_each_begin;
	wtext get_name() const override { return L"token_begin_loop_for"; }
	void perform(space * spc, ast::prepare_data * pd, base_log * log) override;
};

struct token_loop_for_end : public token_scope_end {
	wtext get_name() const override { return L"token_end_loop_for"; }
};

struct token_loop_for_key : public token_loop_each_key {
	using token_loop_each_key::token_loop_each_key;
	wtext get_name() const override { return L"token_loop_for_key"; }
};

struct token_loop_for_val : public base_token {
	wtext var_name_;

	token_loop_for_val(const wtext & var_name) : var_name_(var_name) {}
	wtext get_name() const override { return L"token_loop_for_val"; }
	void perform(space * spc, ast::prepare_data * pd, base_log * log) override;
};

struct token_loop_for_also_block : public token_loop_each_also_block {
	wtext get_name() const override { return L"token_loop_for_also_block"; }
};

struct token_loop_for_body : public token_loop_each_body {
	using token_loop_each_body::token_loop_each_body;
	wtext get_name() const override { return L"token_loop_for_body"; }
};

struct token_loop_for_body_rest : public token_loop_each_body_rest {
	wtext get_name() const override { return L"token_loop_for_body_rest"; }
};

// keywords allowed inside loop body

struct token_kw_next_or_break : public base_token {
	const mod::instr_type * i_type_;
	also::t_pos pos_;
	id depth_;

	token_kw_next_or_break(
		const mod::instr_type * i_type, const also::t_pos & pos, id depth
	) : i_type_(i_type), pos_(pos), depth_(depth) {}
	void perform(space * spc, ast::prepare_data * pd, base_log * log) override;
};

struct token_kw_next : public token_kw_next_or_break {
	using token_kw_next_or_break::token_kw_next_or_break;
	wtext get_name() const override { return L"token_kw_next"; }
};

struct token_kw_break : public token_kw_next_or_break {
	using token_kw_next_or_break::token_kw_next_or_break;
	wtext get_name() const override { return L"token_kw_break"; }
};

// other

struct token_next_expr : public base_token {
	token_next_expr() = default;
	wtext get_name() const override { return L"token_next_expr"; }
	void perform(space * spc, ast::prepare_data * pd, base_log * log) override;
};

struct token_add_op : public base_token {
	also::t_pos pos_;
	const ast::op_info_lite * op_;

	token_add_op(also::t_pos pos, const ast::op_info_lite * op) : pos_(pos), op_(op) {}
	wtext get_name() const override { return L"token_add_operator"; }
	void perform(space * spc, ast::prepare_data * pd, base_log * log) override;
};

struct token_add_fn_native : public base_token {
	also::t_pos pos_;
	wtext fn_name_;
	bool is_bk_;

	token_add_fn_native(also::t_pos pos, const wtext & fn_name, bool is_bk) : pos_(pos), fn_name_(fn_name), is_bk_(is_bk) {}
	wtext get_name() const override { return L"token_add_native_fn"; }
	void perform(space * spc, ast::prepare_data * pd, base_log * log) override;
};

struct token_add_fn_global : public base_token {
	also::t_pos pos_;
	wtext fn_name_;
	bool is_bk_;

	token_add_fn_global(also::t_pos pos, const wtext & fn_name, bool is_bk) : pos_(pos), fn_name_(fn_name), is_bk_(is_bk) {}
	wtext get_name() const override { return L"token_add_global_fn"; }
	void perform(space * spc, ast::prepare_data * pd, base_log * log) override;
};

} // ns
} // ns
