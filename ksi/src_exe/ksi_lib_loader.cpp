#include "ksi_lib_loader.h"
#include <windows.h>

namespace ksi {

const api * lib_loader::load_and_get_api(const ex::wtext & file) {
	const api * ret = nullptr;
	ex::wtext path = calc_path(file);
	if( HMODULE hl = LoadLibraryW(path.h_->cs_) ) {
		bool need_unload = false;
		fn_get_api fn = reinterpret_cast<fn_get_api>( GetProcAddress(hl, "get_ksi_api") );
		if( fn == NULL ) need_unload = true; else ret = fn();
		if( need_unload ) FreeLibrary(hl);
	}
	return ret;
}

ex::wtext lib_loader::calc_path(const ex::wtext & file) {
	ex::wtext ret = L"ksi.dll";
	if( file ) {
		if( HMODULE mod = GetModuleHandleW(file.h_->cs_) ) {
			static constexpr DWORD buf_size = 1024;
			ex::wtext::Char buf[buf_size];
			if(
				ex::id len = GetModuleFileNameW(mod, buf, buf_size);
				len && GetLastError() != ERROR_INSUFFICIENT_BUFFER
			) {
				ex::wtext path_mod(ex::base_text::n_get::val, buf);
				ret = ex::replace_filename(path_mod, ret);
			}
		}
	}
	return ret;
}

} // ns
