#include "../src/pre.h"

import <new>;
import just.output;
import ksi.var;
import ksi.ast;

int main(int p_args_count, char * p_args[], char * p_env[]) {
	//using namespace just::text_literals;
	try {
		//ksi::fs::path v_p = ksi::fs::weakly_canonical("d:/sound/fa1/fb/../../fa2/fb/../t");
		//just::g_console, v_p.c_str(), just::g_new_line;
		if( p_args_count < 2 ) {
			just::g_console, "ksi.exe <path_to_folder>\n";
			return 0;
		}
		ksi::var::g_config->m_path = p_args[0];
		ksi::log_list v_log;
		ksi::var::log_switcher v_log_change{&v_log};
		ksi::space v_space;
		ksi::prepare_data v_data(&v_space);
		if( v_data.load_folder(p_args[1], &v_log) != ksi::file_status::loaded ) {
			just::g_console, "error: Unable to load path: ", p_args[1], just::g_new_line;
			v_log.out(just::g_console);
		}
		// test
		ksi::var::any_var v_var{true};
		just::g_console, &v_var, just::g_new_line;
	} catch( const std::bad_alloc & e ) {
		just::g_console, "error: Memory allocation; ", e.what(), just::g_new_line;
	} catch( ... ) {
		just::g_console, "error: Unexpected.", just::g_new_line;
	}
	return 0;
}