module;

#include "../src/pre.h"

export module ksi.ast;

import <concepts>;
import just.files;
export import ksi.config;

export namespace ksi {

	using namespace just::text_literals;

	using file_read_result = just::result<just::text>;

	file_read_result file_read(const fs::path & p_path, log_base::pointer p_log, t_int_ptr & p_error_count) {
		just::file_read_status v_read_status;
		file_read_result v_ret{false, just::file_read(p_path, v_read_status)};
		switch( v_read_status ) {
		case just::file_read_status::fail_get_size :
			++p_error_count;
			p_log->add({p_path, "error: Unable to get file size."_jt});
			break;
		case just::file_read_status::fail_open :
			++p_error_count;
			p_log->add({p_path, "error: Unable to open file."_jt});
			break;
		case just::file_read_status::fail_read :
			++p_error_count;
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
			return m_structs.m_zero.node_empty() ? nullptr : m_structs.m_zero.m_prev->node_target();
		}

		bool struct_add(t_integer & p_id, const t_text_value & p_name, bool p_is_local, const log_pos & p_log_pos) {
			var::type_struct_pointer v_struct = new var::type_struct(p_name, m_module, p_id, p_is_local, p_log_pos);
			m_structs.append(v_struct);
			return v_struct->m_is_added = type_reg(v_struct);
		}
	};

	enum class file_status {
		loaded,
		in_process,
		with_error,
		absent,
		unknown
	};

	struct type_extend_info {
		// data
		position					m_pos;
		t_text_value				m_type_name;
		t_text_value				m_module_name;
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
		using t_type_extends = std::vector<type_extend_info>;

		// data
		t_space_pointer				m_space;
		log_base::pointer			m_log;
		t_files						m_files;
		t_int_ptr					m_error_count = 0;
		t_ext_modules_list			m_ext_modules_list;
		t_ext_modules_map			m_ext_modules_map;
		module_extension::pointer	m_ext_module_current = nullptr;
		module_extension::pointer	m_ext_module_global = nullptr;
		t_text_value				m_type_name;
		bool						m_type_is_local;
		log_pos						m_type_pos;
		t_type_extends				m_type_extends;
		t_integer					m_id_struct = var::n_id_struct;

		prepare_data(t_space_pointer p_space, log_base::pointer p_log) : m_space{p_space}, m_log{p_log} {
			m_ext_module_global = ext_module_open("@global#"_jt);
		}

		void error(log_message && p_message) {
			++m_error_count;
			m_log->add(std::move(p_message) );
		}

		module_extension::pointer ext_module_find(const t_text_value & p_module_name) {
			typename t_ext_modules_map::iterator v_it = m_ext_modules_map.find(p_module_name);
			return (v_it == m_ext_modules_map.end() ) ? nullptr : (*v_it).second;
		}

		bool struct_add() {
			bool ret{ m_ext_module_current->struct_add(m_id_struct, m_type_name, m_type_is_local, m_type_pos) };
			if( ! m_type_is_local ) {
				var::type_struct_pointer v_struct = m_ext_module_current->last_struct();
				v_struct->m_is_global = m_ext_module_global->type_reg(v_struct);
			}
			return ret;
		}

		template <typename T_module>
		var::type_pointer impl_type_find(T_module * p_module, const type_extend_info & p_type_extend) {
			var::type_pointer v_ret = p_module->type_find(p_type_extend.m_type_name);
			if( v_ret == nullptr ) {
				error({m_type_pos.m_path, just::implode<t_text_value::type>(
					{"deduce error: Type not yet defined: ", p_type_extend.m_type_name, p_type_extend.m_module_name}
				), p_type_extend.m_pos});
			}
			return v_ret;
		}

		var::type_pointer type_find(const type_extend_info & p_type_extend) {
			module_extension::pointer v_ext_module = nullptr;
			switch( p_type_extend.m_module_name.size() ) {
			case 0:
				v_ext_module = m_ext_module_global;
				break;
			case 1:
				v_ext_module = m_ext_module_current;
				break;
			default:
				v_ext_module = ext_module_find(p_type_extend.m_module_name);
				if( v_ext_module == nullptr ) {
					module_space::pointer v_module = m_space->module_find(p_type_extend.m_module_name);
					if( v_module == nullptr ) {
						error({m_type_pos.m_path, just::implode<t_text_value::type>(
							{"deduce error: Module not yet defined: ", p_type_extend.m_module_name}
						), p_type_extend.m_pos});
						return nullptr;
					}
					return impl_type_find(v_module, p_type_extend);
				}
			}
			return impl_type_find(v_ext_module, p_type_extend);
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
			m_modules_map.try_emplace(p_name, v_module);
			return v_module;
		}

		module_extension::pointer ext_module_open(const t_text_value & p_name) {
			if( typename t_ext_modules_map::iterator v_it = m_ext_modules_map.find(p_name);
				v_it != m_ext_modules_map.end()
			) {
				return (*v_it).second;
			}
			module_extension::pointer v_ext_module = new module_extension{module_get(p_name)};
			m_ext_modules_list.append(v_ext_module);
			m_ext_modules_map.try_emplace(p_name, v_ext_module);
			return v_ext_module;
		}

		void apply() {
			if( m_error_count ) { return; }
			m_ext_modules_list.m_zero.node_apply_to_others([](t_ext_modules_list::t_node_pointer p_node){
				p_node->node_target()->apply();
			});
			m_modules_list.m_zero.node_apply_to_others([this](t_modules_list::t_node_pointer p_node){
				module_space::pointer v_module = p_node->node_target();
				m_space->m_modules_map.try_emplace(v_module->m_name, v_module);
			});
			m_space->m_modules_list.splice(m_modules_list);
			for( typename t_files::value_type & v_it : m_files ) {
				if( v_it.second == file_status::loaded ) {
					m_space->m_files.emplace(v_it.first);
				}
			}
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