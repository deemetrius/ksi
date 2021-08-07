#include "ksi_api.h"

namespace ksi {

std::wostream * get_wc();

void init(const api * v_api);
bool run_script(const ex::wtext & path, const run_args & ra, base_log * log);

namespace mod {

void native_wrap(fn_native_can_throw fne, space * spc, fn_space * fns, t_stack * stk, base_log * log);
std::exception_ptr call_func_from_native(
	base_func * fn, var::any & ret,
	space * spc, fn_space * caller, t_stack * stk,
	const also::t_pos & pos, base_log * log,
	const var::any & v1, const var::any & v2, const var::any & v3
);

} // ns

namespace also {

ex::wtext decode(const char * str, ex::id src_len);
ex::text encode(const wchar_t * str, ex::id src_len);

} // ns

const api * get_api() {
	static ksi::api ret;
	static bool need_init = true;
	if( need_init ) {
		need_init = false;
		ret.fn_native_wrap_				= mod::native_wrap;
		ret.fn_call_func_from_native_	= mod::call_func_from_native;
		ret.fn_run_script_				= run_script;
		ret.fn_get_config_				= var::get_config;
		ret.fn_get_wc_					= get_wc;
		ret.fn_decode_					= also::decode;
		ret.fn_encode_					= also::encode;
		ksi::init(&ret);
	}
	return &ret;
}

} // ns

#ifdef __cplusplus
extern "C" {
#endif
__declspec(dllexport) const ksi::api * __cdecl get_ksi_api() {
	return ksi::get_api();
}
#ifdef __cplusplus
}
#endif
