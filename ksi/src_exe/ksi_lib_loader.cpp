#include "ksi_lib_loader.h"
#include <windows.h>

namespace ksi {

const api * lib_loader::load_and_get_api() {
	const api * ret = nullptr;
	if( HMODULE hl = LoadLibraryW(L"ksi.dll"); hl != NULL ) {
		bool need_unload = false;
		fn_get_api fn = reinterpret_cast<fn_get_api>( GetProcAddress(hl, "get_ksi_api") );
		if( fn == NULL ) need_unload = true; else ret = fn();
		if( need_unload ) FreeLibrary(hl);
	}
	return ret;
}

} // ns
