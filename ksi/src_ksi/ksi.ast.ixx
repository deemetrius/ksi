module;

#include "../src/pre.h"

export module ksi.ast;

import <concepts>;
import just.files;
export import ksi.config;

export namespace ksi {

	using namespace just::text_literals;

	using file_read_result = just::result<just::text>;

	file_read_result file_read(const fs::path & p_path, log_base::pointer p_log) {
		just::file_read_status v_read_status;
		file_read_result v_ret{false, just::file_read(p_path, v_read_status)};
		switch( v_read_status ) {
		case just::file_read_status::fail_get_size :
			p_log->add({p_path, "error: Unable to get file size."_jt});
			break;
		case just::file_read_status::fail_open :
			p_log->add({p_path, "error: Unable to open file."_jt});
			break;
		case just::file_read_status::fail_read :
			p_log->add({p_path, "error: Unable to read file."_jt});
			break;
		default:
			v_ret.m_nice = true;
		}
		return v_ret;
	}

	struct module_extension :
		public module_base,
		public just::node_list<module_extension>,
		public just::bases::with_deleter<module_extension *>
	{
		using pointer = module_extension *;

		// data
		module_space::pointer	m_module;

		module_extension(module_space::pointer p_module) : m_module{p_module} {
			init<true>();
		}

		t_text_value name() const override { return m_module->m_name; }

		template <bool C_types>
		void init() {
			if constexpr( C_types ) {
				m_types = m_module->m_types;
			}
			for( typename t_types_used::t_info && v_it : m_module->m_types_used ) {
				m_types_used.maybe_emplace(v_it.m_key, *v_it.m_value);
			}
		}

		void apply() {
			m_module->m_types = m_types;
			std::ranges::swap(m_types_used, m_module->m_types_used);
			m_types_used.clear();
			init<false>();
			m_module->m_structs.splice(m_structs);
		}

		var::type_struct_pointer last_struct() {
			return m_structs.m_zero.node_empty() ? nullptr : m_structs.m_zero.m_prev->node_get_target();
		}

		bool struct_add(const t_text_value & p_name, bool p_is_local, const log_pos & p_log_pos) {
			var::type_struct_pointer v_struct = new var::type_struct(p_name, m_module, p_is_local, p_log_pos);
			m_structs.append(v_struct);
			typename t_types_insert v_res = m_types.emplace(p_name, v_struct);
			return v_res.second;
		}
	};

	enum class file_status {
		loaded,
		in_process,
		with_error,
		absent,
		unknown
	};

	struct prepare_data :
		public space_base
	{
		using pointer = prepare_data *;
		using t_files = std::map<fs::path, file_status>;
		using t_space_pointer = space *;
		using t_ext_modules_map = std::map<t_text_value, module_extension::pointer, just::text_less>;
		using t_ext_modules_list = just::list<module_extension,
			just::closers::compound_call_deleter<false>::template t_closer
		>;

		// data
		t_space_pointer				m_space;
		log_base::pointer			m_log;
		t_files						m_files;
		t_int_ptr					m_error_count = 0;
		t_ext_modules_list			m_ext_modules_list;
		t_ext_modules_map			m_ext_modules_map;
		module_extension::pointer	m_ext_module_current = nullptr;
		t_text_value				m_type_name;
		log_pos						m_type_pos;
		bool						m_type_is_local;

		prepare_data(t_space_pointer p_space, log_base::pointer p_log) : m_space{p_space}, m_log{p_log} {}

		void error(log_message && p_message) {
			++m_error_count;
			m_log->add(std::move(p_message) );
		}

		module_space::pointer module_get(const t_text_value & p_name) {
			if( typename t_modules_map::iterator v_it = m_space->m_modules_map.find(p_name);
				v_it != m_space->m_modules_map.end()
			) {
				return (*v_it).second;
			}
			if( typename t_modules_map::iterator v_it = m_modules_map.find(p_name); v_it != m_modules_map.end() ) {
				return (*v_it).second;
			}
			module_space::pointer v_module = new module_space{p_name};
			m_modules_list.append(v_module);
			m_modules_map.emplace(p_name, v_module);
			return v_module;
		}

		module_extension::pointer ext_module_open(const t_text_value & p_name) {
			if( typename t_ext_modules_map::iterator v_it = m_ext_modules_map.find(p_name);
				v_it != m_ext_modules_map.end()
			) {
				m_ext_module_current = (*v_it).second;
				return m_ext_module_current;
			}
			module_extension::pointer v_ext_module = new module_extension{module_get(p_name)};
			m_ext_module_current = v_ext_module;
			m_ext_modules_list.append(v_ext_module);
			m_ext_modules_map.emplace(p_name, v_ext_module);
			return v_ext_module;
		}

		void apply() {
			if( m_error_count ) { return; }
			m_ext_modules_list.m_zero.node_apply_to_others([](t_ext_modules_list::t_node_pointer p_node){
				p_node->node_get_target()->apply();
			});
			m_space->m_modules_list.splice(m_modules_list);
		}

		file_status check_path(const fs::path & p_path) {
			if( m_space->m_files.contains(p_path) ) {
				return file_status::loaded;
			}
			t_files::iterator v_it = m_files.find(p_path);
			return v_it == m_files.end() ? file_status::unknown : (*v_it).second;
		}

		file_status load_folder(const fs::path & p_path);
		file_status load_file(const fs::path & p_path);

	private:
		file_status folder_fail(
			file_status p_status,
			const fs::path & p_path,
			const fs::path p_priority_path,
			const just::text & p_message,
			just::t_index p_line,
			just::t_index p_char
		) {
			error({ p_priority_path, p_message, {p_line, p_char} });
			m_files.insert_or_assign(p_priority_path, p_status);
			m_files.insert_or_assign(p_path, p_status);
			return p_status;
		}
	};

	bool is_wrong_char(char p_char) {
		return just::is_one_of(p_char, '\t', '"', ':', '/', '\\', '*', '?', '<', '>', '|');
	}

} // ns