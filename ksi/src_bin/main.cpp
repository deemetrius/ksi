#include "../src/pre.h"

import <new>;
import just.output;
import ksi.var;
import ksi.space;

int main(int p_args_count, char * p_args[], char * p_env[]) {
	//using namespace just::text_literals;
	try {
		ksi::var::g_config->m_path = p_args[0];
		if( p_args_count < 2 ) {
			just::g_console, "ksi.exe <path_to_folder>\n";
			return 0;
		}
		ksi::log_list v_log;
		ksi::space v_space;
		if( ksi::load_folder(v_space, p_args[1], &v_log) == ksi::load_status::not_loaded ) {
			just::g_console, "error: Unable to load path: ", p_args[1], just::g_new_line;
			v_log.out(just::g_console);
		}
	} catch( const std::bad_alloc & e ) {
		just::g_console, "error: Memory allocation; ", e.what(), just::g_new_line;
	} catch( ... ) {
		just::g_console, "error: Unexpected.", just::g_new_line;
	}
	return 0;
}