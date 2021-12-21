#include "ksi_lib_loader.h"
#include "../src/ksi_types.h"
#include <clocale>
#include <new>

int wmain(int args_count, wchar_t ** args, wchar_t ** env) {
	std::setlocale(LC_ALL, "Rus");
	if( args_count < 2 ) {
		std::wcout << L"Usage: ksi.exe file.ksi [-debug -show_log]" << ksi::endl;
		return 0;
	}
	try {
		if( const ksi::api * api = ksi::lib_loader::load_and_get_api() ) {
			ksi::run_args ra;
			ra.init(args, args_count);
			ksi::log_array log;
			ex::wtext path(ex::wtext::n_get::val, args[1]);
			if( !api->fn_run_script_(path, ra, &log) )
			return 1;
		} else {
			std::wcout << L"Unable to load lib: ksi.dll" << ksi::endl;
		}
	} catch( const std::bad_alloc & e ) {
		std::wcout << "Memory allocation error: " << e.what() << ksi::endl;
		return 1;
	} catch( ... ) {
		std::wcout << L"Unknown error." << ksi::endl;
		return 1;
	}
	return 0;
}

// wrapper

extern int _CRT_glob;
extern
#ifdef __cplusplus
"C"
#endif
void __wgetmainargs(int*,wchar_t***,wchar_t***,int,int*);

int main() {
	wchar_t **enpv, **argv;
	int argc, si = 0;
	__wgetmainargs(&argc, &argv, &enpv, _CRT_glob, &si); // this also creates the global variable __wargv
	return wmain(argc, argv, enpv);
}
