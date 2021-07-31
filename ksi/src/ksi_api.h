#pragma once
#include "extra.h"
#include <exception>

namespace ksi {

struct space;
struct t_stack;
struct base_log;
struct run_args;

namespace also {

struct t_pos;

} // ns

namespace var {

struct any;
struct config;
using fn_get_config = const config * (*)();

#ifdef KSI_LIB
const config * get_config();
#endif // KSI_LIB

} // ns

namespace mod {

struct fn_space;
struct base_func;

using fn_native_can_throw = std::exception_ptr (*)(space * spc, fn_space * fns, t_stack * stk, base_log * log);
using fn_native_simple = void (*)(fn_native_can_throw, space * spc, fn_space * fns, t_stack * stk, base_log * log);

using fn_call_func_from_native = std::exception_ptr (*)(
	base_func * fn, var::any & ret,
	space * spc, fn_space * caller, t_stack * stk,
	const also::t_pos & pos, base_log * log,
	const var::any & v1, const var::any & v2, const var::any & v3
);

} // ns

using fn_run_script = bool (*)(const ex::wtext & path, const run_args & ra, base_log * log);
using fn_get_wc = std::wostream * (*)();

// api

struct api {
	mod::fn_native_simple			fn_native_wrap_;
	mod::fn_call_func_from_native	fn_call_func_from_native_;
	fn_run_script					fn_run_script_;
	var::fn_get_config				fn_get_config_;
	fn_get_wc						fn_get_wc_;
};

using fn_get_api = const ksi::api * __cdecl (*)();

} // ns
