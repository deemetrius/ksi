#include "../src/pre.h"

import <new>;
import just.output;

int main(int p_args_count, char * p_args[], char * p_env[]) {
	try {
		if( p_args_count < 2 ) {
			just::g_console << "ksi.exe <path_to_folder>\n";
			//std::getchar();
			return 0;
		}
		//
	} catch( const std::bad_alloc & e ) {
		just::g_console << "error: Memory allocation; " << e.what() << just::g_new_line;
	} catch( ... ) {
		just::g_console << "error: Unexpected." << just::g_new_line;
	}
	return 0;
}