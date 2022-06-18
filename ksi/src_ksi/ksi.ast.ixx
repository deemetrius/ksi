module;

#include "../src/pre.h"

export module ksi.ast;

import <concepts>;
export import <map>;
export import ksi.space;
import just.files;

export namespace ksi {

	struct module_extension :
		public module_base
	{
		// data
		//t_index		m_type_position = 0;

		void types_add_from_module(module_base & p_module) {
			for (var::type_pointer v_it : p_module.m_types.m_vector) {
				m_types.maybe_emplace(v_it->m_name, v_it);
			}
			//m_type_position = m_types.m_vector.size();
		}

		void types_put_to_module(module_base & p_module) {
			/*for( t_index v_index = m_type_position, v_count = m_types.m_vector.size(); v_index < v_count; ++v_index ) {
				var::type_pointer v_type = m_types.m_vector[v_index];
				p_module.m_types.maybe_emplace(v_type->m_name, v_type);
			}*/
			std::ranges::swap(m_types, p_module.m_types);
		}
	};

	enum class file_status {
		loaded,
		in_process,
		with_error,
		absent,
		unknown
	};

	struct prepare_data {
		using t_files = std::map<fs::path, file_status>;
		using t_space_pointer = space *;

		// data
		t_space_pointer		m_space;
		t_files				m_files;

		prepare_data(t_space_pointer p_space) : m_space{p_space} {}

		file_status check_path(const fs::path & p_path) {
			if( m_space->m_files.contains(p_path) ) {
				return file_status::loaded;
			}
			t_files::iterator v_it = m_files.find(p_path);
			return v_it == m_files.end() ? file_status::unknown : (*v_it).second;
		}
	};

	//enum class load_status { already_loaded, just_loaded, not_loaded };

	file_status load_folder(prepare_data & p_data, const fs::path & p_path, log_base::pointer p_log) {
		using namespace just::text_literals;
		if( file_status v_res = p_data.check_path(p_path); v_res != file_status::unknown ) {
			return v_res;
		}
		if( fs::file_type v_file_type = just::file_type(p_path); v_file_type != fs::file_type::directory ) {
			file_status v_ret = (v_file_type == fs::file_type::not_found) ? file_status::absent : file_status::with_error;
			p_data.m_files.insert_or_assign(p_path, v_ret);
			p_log->add(log_message{ p_path, "error: Given path should be directory."_jt });
			return v_ret;
		}
		fs::path v_priority_path = p_path;
		v_priority_path.append("define.priority");
		if( fs::file_type v_file_type = just::file_type(v_priority_path); v_file_type != fs::file_type::regular ) {
			file_status v_ret = (v_file_type == fs::file_type::not_found) ? file_status::absent : file_status::with_error;
			p_data.m_files.insert_or_assign(v_priority_path, v_ret);
			p_data.m_files.insert_or_assign(p_path, file_status::with_error);
			p_log->add(log_message{ v_priority_path, "error: Given file should exists."_jt });
			return v_ret;
		}
		p_data.m_files.insert_or_assign(p_path, file_status::in_process);
		p_data.m_files.insert_or_assign(v_priority_path, file_status::in_process);
		//
		//
		p_data.m_files.insert_or_assign(p_path, file_status::loaded);
		p_data.m_files.insert_or_assign(v_priority_path, file_status::loaded);
		return file_status::loaded;
	}

} // ns