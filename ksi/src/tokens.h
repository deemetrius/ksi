#pragma once
#include "ksi_types.h"

namespace ksi {

struct space;

namespace ast {

struct prepare_data;

} // ns
namespace tokens {

struct base_token {
	virtual ~base_token() = default;
	virtual wtext get_name() const = 0;
	virtual void prepare(space * spc, ast::prepare_data * pd, base_log * log) {}
	virtual void perform(space * spc, ast::prepare_data * pd, base_log * log) {}
};

} // ns

using t_tokens = ex::def_array<tokens::base_token *, ex::del_pointer, def::tokens_r, def::tokens_s>;

} // ns
