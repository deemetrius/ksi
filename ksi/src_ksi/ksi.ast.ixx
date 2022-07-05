module;

#include "../src/pre.h"

export module ksi.ast;

import <concepts>;
import just.files;
export import ksi.config;

export namespace ksi {

	using file_read_result = just::result<just::text>;

	file_read_result file_read(const fs::path & p_path, log_base::pointer p_log) {
		using namespace just::text_literals;
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

		module_extension(module_space::pointer p_module) : m_module{p_module} {}

		t_text_value name() const override { return m_module->m_name; }

		void init() {
			for( typename t_types::t_info && v_it : m_module->m_types_used ) {
				m_types_used.maybe_emplace(v_it.m_key, *v_it.m_value);
			}
			for( typename t_types::t_info && v_it : m_module->m_types ) {
				m_types.maybe_emplace(v_it.m_key, *v_it.m_value);
			}
		}

		void apply() {
			std::ranges::swap(m_types_used, m_module->m_types_used);
			m_types_used.clear();
			std::ranges::swap(m_types, m_module->m_types);
			m_types.clear();
			init();
			m_module->m_structs.splice(m_structs);
		}

		var::type_struct_pointer last_struct() {
			return m_structs.m_zero.node_empty() ? nullptr : m_structs.m_zero.m_prev->node_get_target();
		}

		bool struct_add(const t_text_value & p_name, const log_pos & p_log_pos) {
			var::type_struct_pointer v_struct = new var::type_struct(p_name, m_module, p_log_pos);
			m_structs.m_zero.m_prev->node_attach(v_struct);
			typename t_types::t_find_result v_res = m_types.maybe_emplace(p_name, v_struct);
			return v_res.m_added;
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
		using t_ext_modules_map = just::hive<t_text_value, module_extension::pointer, just::text_less>;
		using t_ext_modules_list = just::list<module_extension,
			just::closers::compound_call_deleter<false>::template t_closer
		>;

		// data
		t_space_pointer				m_space;
		log_base::pointer			m_log;
		t_files						m_files;
		t_index						m_error_count = 0;
		t_ext_modules_list			m_ext_modules_list;
		t_ext_modules_map			m_ext_modules_map;
		module_extension::pointer	m_ext_module_current = nullptr;
		t_text_value				m_type_name;
		log_pos						m_type_pos;

		prepare_data(t_space_pointer p_space, log_base::pointer p_log) : m_space{p_space}, m_log{p_log} {}

		void error(log_message && p_message) {
			++m_error_count;
			m_log->add(std::move(p_message) );
		}

		module_space::pointer module_get(const t_text_value & p_name) {
			if( typename t_modules_map::t_find_result v_res = m_space->m_modules_map.find(p_name); v_res.m_added ) {
				return *v_res.m_value;
			}
			if( typename t_modules_map::t_find_result v_res = m_modules_map.find(p_name); v_res.m_added ) {
				return *v_res.m_value;
			}
			module_space::pointer v_module = new module_space{p_name};
			m_modules_list.m_zero.m_prev->node_attach(v_module);
			m_modules_map.maybe_emplace(p_name, v_module);
			return v_module;
		}

		module_extension::pointer ext_module_open(const t_text_value & p_name) {
			if( typename t_ext_modules_map::t_find_result v_res = m_ext_modules_map.find(p_name); v_res.m_added ) {
				m_ext_module_current = *v_res.m_value;
				return *v_res.m_value;
			}
			module_extension::pointer v_ext_module = new module_extension{module_get(p_name)};
			m_ext_module_current = v_ext_module;
			m_ext_modules_list.m_zero.m_prev->node_attach(v_ext_module);
			m_ext_modules_map.maybe_emplace(p_name, v_ext_module);
			return v_ext_module;
		}

		file_status check_path(const fs::path & p_path) {
			if( m_space->m_files.contains(p_path) ) {
				return file_status::loaded;
			}
			t_files::iterator v_it = m_files.find(p_path);
			return v_it == m_files.end() ? file_status::unknown : (*v_it).second;
		}

		file_status load_folder(const fs::path & p_path, log_base::pointer p_log);
		file_status load_file(const fs::path & p_path, log_base::pointer p_log);

	private:
		file_status folder_fail(
			file_status p_status,
			const fs::path & p_path,
			const fs::path p_priority_path,
			log_base::pointer p_log,
			const just::text & p_message,
			just::t_int p_line,
			just::t_int p_char
		) {
			p_log->add({ p_priority_path, p_message, {p_line, p_char} });
			m_files.insert_or_assign(p_priority_path, p_status);
			m_files.insert_or_assign(p_path, p_status);
			return p_status;
		}
	};

	bool is_wrong_char(char p_char) {
		return just::is_one_of(p_char, '\t', '"', ':', '/', '\\', '*', '?', '<', '>', '|');
	}

	file_status prepare_data::load_folder(const fs::path & p_path, log_base::pointer p_log) {
		using namespace just::text_literals;
		fs::path v_path = fs::weakly_canonical(p_path);
		if( file_status v_res = check_path(v_path); v_res != file_status::unknown ) {
			return v_res;
		}
		if( fs::file_type v_file_type = just::file_type(v_path); v_file_type != fs::file_type::directory ) {
			++m_error_count;
			file_status v_ret = (v_file_type == fs::file_type::not_found) ? file_status::absent : file_status::with_error;
			m_files.insert_or_assign(v_path, v_ret);
			p_log->add({ v_path, "error: Given path should be directory."_jt });
			return v_ret;
		}
		fs::path v_priority_path = v_path;
		v_priority_path.append("define.priority");
		if( fs::file_type v_file_type = just::file_type(v_priority_path); v_file_type != fs::file_type::regular ) {
			++m_error_count;
			file_status v_ret = (v_file_type == fs::file_type::not_found) ? file_status::absent : file_status::with_error;
			return folder_fail(v_ret, v_path, v_priority_path, p_log, "error: Given file should exists."_jt, 0, 0);
		}
		m_files.insert_or_assign(v_path, file_status::in_process);
		m_files.insert_or_assign(v_priority_path, file_status::in_process);
		//
		file_read_result v_file_res = file_read(v_priority_path, p_log);
		if( ! v_file_res ) {
			m_files.insert_or_assign(v_path, file_status::with_error);
			m_files.insert_or_assign(v_priority_path, file_status::with_error);
			return file_status::with_error;
		}
		just::text v_ext = ".define"_jt;
		just::t_plain_text v_text = v_file_res.m_value->m_text;
		just::t_plain_text v_start_line = v_text;
		just::t_int v_line = 1;
		just::text::t_const_pointer v_cmd_ext = "ext: \""_jt;
		fs::path v_file_path;
		do {
			if( v_text == v_start_line ) {
				if( just::text_traits::cmp_n(v_text, v_cmd_ext->m_text, v_cmd_ext->m_length) == 0 ) {
					// ext: ""
					v_text += v_cmd_ext->m_length;
					just::t_plain_text v_start_text = v_text;
					while( *v_text != '"' ) {
						if( *v_text == '\0' ) {
							return folder_fail(file_status::with_error, v_path, v_priority_path, p_log,
								"error: Unexpected end of file."_jt, v_line, v_text - v_start_line +1
							);
						}
						if( just::is_one_of(*v_text, '\r', '\n') ) {
							return folder_fail(file_status::with_error, v_path, v_priority_path, p_log,
								"error: Unexpected end of line."_jt, v_line, v_text - v_start_line +1
							);
						}
						++v_text;
					}
					v_ext = just::text_traits::from_range(v_start_text, v_text);
					++v_text;
					v_start_line = v_text;
				} else if( *v_text == ':' ) {
					// comment
					++v_text;
					while( ! just::is_one_of(*v_text, '\r', '\n', '\0') ) {
						++v_text;
					}
					v_start_line = v_text;
				} else if( *v_text == '"' ) {
					// quoted path
					++v_text;
					just::t_plain_text v_start_text = v_text;
					while( *v_text != '"' ) {
						if( *v_text == '\0' ) {
							return folder_fail(file_status::with_error, v_path, v_priority_path, p_log,
								"error: Unexpected end of file."_jt, v_line, v_text - v_start_line +1
							);
						}
						if( just::is_one_of(*v_text, '\r', '\n') ) {
							return folder_fail(file_status::with_error, v_path, v_priority_path, p_log,
								"error: Unexpected end of line."_jt, v_line, v_text - v_start_line +1
							);
						}
						++v_text;
					}
					just::text v_file = just::text_traits::from_range(v_start_text, v_text);
					v_file_path += static_cast<std::string_view>(v_file);
					if( v_file_path.is_relative() ) {
						fs::path v_quoted_path = v_file_path;
						v_file_path = v_path;
						v_file_path /= v_quoted_path;
					}
					v_file_path = fs::weakly_canonical(v_file_path);
					++v_text;
					v_start_line = v_text;
				}
			}
			if( just::is_one_of(*v_text, '\r', '\n', '\0') && (v_text > v_start_line) ) {
				just::text v_name = just::text_traits::from_range(v_start_line, v_text);
				v_file_path = v_path;
				v_file_path /= static_cast<std::string_view>(v_name);
				v_file_path += static_cast<std::string_view>(v_ext);
			}
			if( ! v_file_path.empty() ) {
				file_status v_file_status = load_file(v_file_path, p_log);
				switch( v_file_status ) {
				case file_status::absent :
					{
						just::text v_msg = "error: Given file is absent: "_jt;
						std::string v_file_string = v_file_path.string();
						return folder_fail(file_status::with_error, v_path, v_priority_path, p_log,
							just::implode<char>({v_msg, v_file_string}), v_line, 1
						);
					}
				case file_status::with_error :
					{
						just::text v_msg = "error: Given file contains error: "_jt;
						std::string v_file_string = v_file_path.string();
						return folder_fail(file_status::with_error, v_path, v_priority_path, p_log,
							just::implode<char>({v_msg, v_file_string}), v_line, 1
						);
					}
				}
				v_file_path.clear();
			}
			if( *v_text == '\0' ) { break; }
			if( just::is_one_of(*v_text, '\r', '\n') ) {
				if( *v_text == '\r' && v_text[1] == '\n' ) {
					v_text += 2;
				} else {
					++v_text;
				}
				v_start_line = v_text;
				++v_line;
				continue;
			}
			if( is_wrong_char(*v_text) ) {
				return folder_fail(file_status::with_error, v_path, v_priority_path, p_log,
					"error: Wrong symbol was found."_jt, v_line, v_text - v_start_line +1
				);
			}
			++v_text;
		} while( true );
		//
		m_files.insert_or_assign(v_path, file_status::loaded);
		m_files.insert_or_assign(v_priority_path, file_status::loaded);
		return file_status::loaded;
	}

	file_status prepare_data::load_file(const fs::path & p_path, log_base::pointer p_log) {
		just::g_console, p_path.c_str(), just::g_new_line;
		return file_status::loaded;
	}

} // ns