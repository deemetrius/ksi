#include <windows.h>

extern "C" __declspec(dllexport) BOOL APIENTRY DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
	switch( fdwReason ) {
	case DLL_PROCESS_ATTACH:
		// attach to process
		// return FALSE to fail DLL load
		break;
	case DLL_PROCESS_DETACH:
		// detach from process
		break;
	case DLL_THREAD_ATTACH:
		// attach to thread
		break;
	case DLL_THREAD_DETACH:
		// detach from thread
		break;
	}
	return TRUE; // succesful
}

/*#include "ksi_lib.h"
#include <clocale>
#include <new>

int wmain(int args_count, wchar_t ** args, wchar_t ** env) {
	std::setlocale(LC_ALL, "Rus");
	if( args_count < 2 ) {
		std::wcout << L"Usage: ksi.exe file.ksi [-debug -show_log]" << std::endl;
		return 0;
	}
	try {
		//ksi::init();
		ksi::get_api();
		ksi::run_args ra;
		ra.init(args, args_count);
		ksi::log_array log;
		ex::wtext path(ex::wtext::n_get::val, args[1]);
		if( !ksi::run_script(path, ra, &log) )
		return 1;
	} catch( const std::bad_alloc & e ) {
		std::wcout << "Memory allocation error: " << e.what() << std::endl;
		return 1;
	} catch( ... ) {
		std::wcout << L"Unknown error." << std::endl;
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
*/
