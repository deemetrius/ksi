module;

#include "../src/pre.h"

export module ksi.ast;

export import <map>;
export import ksi.log;
export import ksi.function;

export namespace ksi {

	struct module_extension :
		public module_base
	{
		// data
		t_index		m_type_position = 0;

		void types_add_from_module(module_base & p_module) {
			for (var::type_pointer v_it : p_module.m_types.m_vector) {
				m_types.maybe_emplace(v_it->m_name, v_it);
			}
			m_type_position = m_types.m_vector.size();
		}

		void types_put_to_module(module_base & p_module) {
			for( t_index v_index = m_type_position, v_count = m_types.m_vector.size(); v_index < v_count; ++v_index ) {
				var::type_pointer v_type = m_types.m_vector[v_index];
				p_module.m_types.maybe_emplace(v_type->m_name, v_type);
			}
		}
	};

	enum class file_status {
		loaded,
		in_process,
		with_error,
		absent
	};

	struct space;

	struct prepare_data {
		using t_files = std::map<fs::path, file_status>;
		using t_space_pointer = space *;

		// data
		t_space_pointer		m_space;
		t_files				m_files;

		prepare_data(t_space_pointer p_space) : m_space{p_space} {}
	};

} // ns