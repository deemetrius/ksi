#include "../src/pre.h"

import <new>;
import <clocale>;
import just.input;
import just.output;
import just.args;
import just.array;
import ksi.space;
import ksi.parser;

//using t_data = just::array<just::t_index, 3>;

int wmain(int p_args_count, wchar_t * p_args[], wchar_t * p_env[]) {
	using namespace std::string_literals;
	try {
		std::setlocale(LC_ALL, "rus.utf8");
		//std::wcout.imbue(std::locale("rus.utf8"));
		if( p_args_count < 2 ) {
			just::fs::path v_bin_path{p_args[0]};
			just::fs::path::string_type v_bin_str = v_bin_path.filename().native();
			just::g_console << v_bin_str.data() << " <path>\n";
			//std::getchar();
			return 0;
		}
		ksi::config::make();
		//
		just::fs::path v_path = just::fs::weakly_canonical(p_args[1]);
		ksi::t_text v_f = just::read_wide(v_path);
		just::g_console << v_f->data() << '\n';
		//
		ksi::log_vector v_log;
		ksi::space v_space;
		ksi::t_integer v_res = ksi::parser::parse(&v_space, v_path, v_f, &v_log);
		just::g_console << (v_res ? "* Parsed" : "* Not parsed") << just::g_new_line;
		v_log.out();
		just::g_console << v_space.m_mod_main;

		/*ksi::var::value v1{&ksi::hcfg->mc_null}, v2{&ksi::hcfg->mt_cat};
		just::g_console << ksi::var::compare(v1, v2) << just::g_new_line;*/

		/*ksi::var::value_array::t_items v_vec;
		ksi::var::value::pointer v_item = v_vec.emplace_back(just::separator{}, ksi::var::marker_array{});
		ksi::var::value_array::t_items_pointer v_arr = ksi::var::value_array::get(v_item);
		v_arr->emplace_back(just::separator{}, *v_vec[0]);*/
	} catch( const std::bad_alloc & e ) {
		just::g_console << "error: Memory allocation; " << e.what() << just::g_new_line;
	} catch( ... ) {
		just::g_console << "error: Unexpected." << just::g_new_line;
	}
	return 0;
}