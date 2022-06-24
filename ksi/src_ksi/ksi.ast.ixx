module;

#include "../src/pre.h"

export module ksi.ast;

import <concepts>;
export import <map>;
export import ksi.space;
import just.files;

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
		t_index				m_error_count = 0;

		prepare_data(t_space_pointer p_space) : m_space{p_space} {}

		file_status check_path(const fs::path & p_path) {
			if( m_space->m_files.contains(p_path) ) {
				return file_status::loaded;
			}
			t_files::iterator v_it = m_files.find(p_path);
			return v_it == m_files.end() ? file_status::unknown : (*v_it).second;
		}

		file_status load_folder(const fs::path & p_path, log_base::pointer p_log);
		file_status load_file(const fs::path & p_path, log_base::pointer p_log);
	};

	//enum class load_status { already_loaded, just_loaded, not_loaded };

	bool check_char(char p_char) {
		switch( p_char ) {
		case '\t'	: return false;
		case '"'	: return false;
		case ':'	: return false;
		case '/'	: return false;
		case '\\'	: return false;
		case '*'	: return false;
		case '?'	: return false;
		case '<'	: return false;
		case '>'	: return false;
		case '|'	: return false;
		}
		return true;
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
			m_files.insert_or_assign(v_priority_path, v_ret);
			m_files.insert_or_assign(v_path, file_status::with_error);
			p_log->add({ v_priority_path, "error: Given file should exists."_jt });
			return v_ret;
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
		do {
			if( v_text == v_start_line ) {
				if( just::text_traits::cmp_n(v_text, v_cmd_ext->m_text, v_cmd_ext->m_length) == 0 ) {
					// ext: ""
					v_text += v_cmd_ext->m_length;
					v_start_line = v_text;
					while( *v_text != '"' ) {
						if( *v_text == 0 ) {
							p_log->add({ v_priority_path, "error: Unexpected end of file."_jt,
								{v_line, v_text - v_start_line +1}
							});
							m_files.insert_or_assign(v_path, file_status::with_error);
							m_files.insert_or_assign(v_priority_path, file_status::with_error);
							return file_status::with_error;
						}
						++v_text;
					}
					v_ext = just::text_traits::from_range(v_start_line, v_text);
					++v_text;
					v_start_line = v_text;
				} else if( just::is_one_of(*v_text, ' ', '\t') ) {
					// comment
					++v_text;
					while( !just::is_one_of(*v_text, '\r', '\n', '\0') ) {
						++v_text;
					}
					v_start_line = v_text;
				}
			}
			if( just::is_one_of(*v_text, '\r', '\n', '\0') ) {
				// file
				if( v_text - v_start_line > 0 ) {
					just::text v_name = just::text_traits::from_range(v_start_line, v_text);
					v_name = just::implode({v_name, v_ext});
					fs::path v_file_path = v_path;
					v_file_path.append(v_name->m_text);
					load_file(v_file_path, p_log);
				}
				if( *v_text == '\0' ) { break; }
				if( *v_text == '\r' && v_text[1] == '\n' ) {
					v_text += 2;
				} else {
					++v_text;
				}
				v_start_line = v_text;
				++v_line;
				continue;
			}
			if( ! check_char(*v_text) ) {
				p_log->add({ v_priority_path, "error: Wrong symbol was found."_jt,
					{v_line, v_text - v_start_line +1}
				});
				m_files.insert_or_assign(v_path, file_status::with_error);
				m_files.insert_or_assign(v_priority_path, file_status::with_error);
				return file_status::with_error;
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