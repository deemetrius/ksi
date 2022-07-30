module ksi.prepare_data;

#include "../src/pre.h"

import ksi.rules;

namespace ksi {

	using namespace just::text_literals;

	file_status prepare_data::load_folder(const fs::path & p_path) {
		fs::path v_path = fs::weakly_canonical(p_path);
		if( file_status v_res = check_path(v_path); v_res != file_status::unknown ) {
			return v_res;
		}
		if( fs::file_type v_file_type = just::file_type(v_path); v_file_type != fs::file_type::directory ) {
			file_status v_ret = (v_file_type == fs::file_type::not_found) ? file_status::absent : file_status::with_error;
			m_files.insert_or_assign(v_path, v_ret);
			error({ v_path, "error: Given path should be directory."_jt });
			return v_ret;
		}
		fs::path v_priority_path = v_path;
		v_priority_path.append("define.priority");
		if( fs::file_type v_file_type = just::file_type(v_priority_path); v_file_type != fs::file_type::regular ) {
			file_status v_ret = (v_file_type == fs::file_type::not_found) ? file_status::absent : file_status::with_error;
			return folder_fail(v_ret, v_path, v_priority_path, "error: Given file should exists."_jt, 0, 0);
		}
		m_files.insert_or_assign(v_path, file_status::in_process);
		m_files.insert_or_assign(v_priority_path, file_status::in_process);
		//
		file_read_result v_file_res = file_read(v_priority_path, m_log, m_error_count);
		if( ! v_file_res ) {
			m_files.insert_or_assign(v_path, file_status::with_error);
			m_files.insert_or_assign(v_priority_path, file_status::with_error);
			return file_status::with_error;
		}
		just::text v_ext = ".define"_jt;
		just::t_plain_text v_text = v_file_res.m_value->m_text;
		just::t_plain_text v_start_line = v_text;
		just::t_int_ptr v_line = 1;
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
							return folder_fail(file_status::with_error, v_path, v_priority_path,
								"error: Unexpected end of file."_jt, v_line, v_text - v_start_line +1
							);
						}
						if( just::is_one_of(*v_text, '\r', '\n') ) {
							return folder_fail(file_status::with_error, v_path, v_priority_path,
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
							return folder_fail(file_status::with_error, v_path, v_priority_path,
								"error: Unexpected end of file."_jt, v_line, v_text - v_start_line +1
							);
						}
						if( just::is_one_of(*v_text, '\r', '\n') ) {
							return folder_fail(file_status::with_error, v_path, v_priority_path,
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
			if( just::is_one_of(*v_text, '\r', '\n', '\0') && (v_text != v_start_line) ) {
				just::text v_name = just::text_traits::from_range(v_start_line, v_text);
				v_file_path = v_path;
				v_file_path /= static_cast<std::string_view>(v_name);
				v_file_path += static_cast<std::string_view>(v_ext);
			}
			if( ! v_file_path.empty() ) {
				file_status v_file_status = load_file(v_file_path);
				switch( v_file_status ) {
				case file_status::absent :
					{
						just::text v_msg = "error: Given file is absent: "_jt;
						std::string v_file_string = v_file_path.string();
						return folder_fail(file_status::with_error, v_path, v_priority_path,
							just::implode<char>({v_msg, v_file_string}), v_line, 1
						);
					}
				case file_status::with_error :
					{
						just::text v_msg = "error: Given file contains error: "_jt;
						std::string v_file_string = v_file_path.string();
						return folder_fail(file_status::with_error, v_path, v_priority_path,
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
				return folder_fail(file_status::with_error, v_path, v_priority_path,
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

	file_status prepare_data::load_file(const fs::path & p_path) {
		if( file_status v_res = check_path(p_path); v_res != file_status::unknown ) {
			return v_res;
		}
		if( fs::file_type v_file_type = just::file_type(p_path); v_file_type != fs::file_type::regular ) {
			file_status v_ret = (v_file_type == fs::file_type::not_found) ? file_status::absent : file_status::with_error;
			m_files.insert_or_assign(p_path, v_ret);
			error({p_path, "error: Given file should exists."_jt});
			return v_ret;
		}
		file_read_result v_file_res = file_read(p_path, m_log, m_error_count);
		if( ! v_file_res ) {
			m_files.insert_or_assign(p_path, file_status::with_error);
			return file_status::with_error;
		}
		m_files.insert_or_assign(p_path, file_status::in_process);
		//
		tokens::nest_tokens v_tokens;
		if( ! rules::parse_declarative(this, p_path, v_file_res.m_value, v_tokens, m_log, var::g_config->m_tab_size) ) {
			++m_error_count;
			m_files.insert_or_assign(p_path, file_status::with_error);
			return file_status::with_error;
		}
		if( m_error_count == 0 ) {
			v_tokens.perform(this);
			if( m_error_count == 0 ) {
				m_files.insert_or_assign(p_path, file_status::loaded);
				return file_status::loaded;
			}
		}
		//
		m_files.insert_or_assign(p_path, file_status::with_error);
		return file_status::with_error;
	}

	namespace tokens {

		void nest_tokens::put_literal_struct_prop_default(
			nest_tokens::pointer p_nest,
			const var::any_var & p_value,
			prepare_data_pointer p_data
		) {
			token_struct_prop_name::pointer v_token = static_cast<token_struct_prop_name::pointer>(
				p_nest->m_types.m_zero.m_prev
			);
			v_token->m_value = p_value;
		}

		void nest_tokens::put_literal_fn_param_default(
			nest_tokens::pointer p_nest,
			const var::any_var & p_value,
			prepare_data_pointer p_data
		) {
			late_token_function_add_param::pointer v_token = static_cast<late_token_function_add_param::pointer>(
				p_data->m_late.m_functions.m_zero.m_prev
			);
			v_token->m_value = p_value;
		}

	} // ns

} // ns