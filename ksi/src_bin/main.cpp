#include "../src/pre.h"

import <new>;
import just.output;
import ksi.prepare_data;
import ksi.var.ops;

int main(int p_args_count, char * p_args[], char * p_env[]) {
	using namespace just::text_literals;
	try {
		if( p_args_count < 2 ) {
			just::g_console << "ksi.exe <path_to_folder>\n";
			//std::getchar();
			return 0;
		}
		ksi::var::g_config = ksi::var::config::instance();
		ksi::var::g_config->init();
		ksi::var::g_config->m_path = p_args[0];
		ksi::log_list v_log;
		ksi::var::log_switcher v_log_change{&v_log};
		//
		{
			//ksi::var::any_var v1 = ksi::var::type_float::s_infinity, v2 = ksi::var::variant_all{};
			//ksi::var::any_var v1 = "hello"_jt, v2 = "hello"_jt;
			//just::g_console << ksi::var::compare(v1, v2) << just::g_new_line;
		}
		//
		ksi::space v_space;
		ksi::prepare_data v_data(&v_space, &v_log);
		if( v_data.load_program(p_args[1]) != ksi::file_status::loaded ) {
			just::g_console << "error: Unable to load path: " << p_args[1] << just::g_new_line;
		} else if( v_data.late() ) {
			v_data.apply();
			/*ksi::module_space::pointer v_module = v_space.m_module_global;
			just::g_console << "Types of @global# :\n";
			for( typename ksi::module_space::t_types::value_type & v_it_type : v_module->m_types ) {
				just::g_console << v_it_type.second->m_name_full << just::g_new_line;
				for( typename ksi::var::category::t_includes::t_node_pointer v_it_cat :
					v_it_type.second->m_categories.m_direct
				) {
					just::g_console << '\t' << v_it_cat->m_value->m_name_full << just::g_new_line;
				}
			}*/
		}
		just::g_console << "Error count: " << v_data.m_error_count << just::g_new_line;
		v_log.out(just::g_console);
	} catch( const std::bad_alloc & e ) {
		just::g_console << "error: Memory allocation; " << e.what() << just::g_new_line;
	} catch( ... ) {
		just::g_console << "error: Unexpected." << just::g_new_line;
	}
	return 0;
}