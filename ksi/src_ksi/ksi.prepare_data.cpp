module ksi.prepare_data;

#include "../src/pre.h"

import ksi.rules;

namespace ksi {

	using namespace just::text_literals;

	file_status prepare_data::load_program(const fs::path & p_path) {
		file_status v_ret = load_folder(p_path);
		if( v_ret == file_status::loaded ) {
			for( t_file_seq::t_node_pointer v_it : m_run_orders ) {
				node_file::pointer v_node = v_it->node_target();
				v_node->m_status = load_seq<true>(v_node->m_folder_path, v_node->m_path);
				if( v_node->m_status != file_status::loaded ) { return v_node->m_status; }
			}
			m_run_orders.clear();
		}
		return v_ret;
	}

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
			return folder_fail<false>(v_ret, v_path, v_priority_path, "error: Given file should exists."_jt, 0, 0);
		}
		m_files.insert_or_assign(v_path, file_status::in_process);
		m_files.insert_or_assign(v_priority_path, file_status::in_process);
		//
		file_status v_priority_status = load_seq<false>(v_path, v_priority_path);
		if( v_priority_status != file_status::loaded ) {
			m_files.insert_or_assign(v_path, file_status::with_error);
			return file_status::with_error;
		}
		m_files.insert_or_assign(v_path, file_status::loaded);
		//
		fs::path v_run_order_path = v_path;
		v_run_order_path.append("run.order");
		run_order_add(v_path, v_run_order_path);
		//
		return file_status::loaded;
	}

	file_status prepare_data::load_file(bool p_is_imperative, const fs::path & p_path) {
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
		if( ! rules::parse_file(this, p_is_imperative, p_path, v_file_res.m_value, v_tokens, m_log, var::g_config->m_tab_size) ) {
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

		void nest_tokens::put_literal_imperative(
			nest_tokens::pointer p_nest,
			const var::any_var & p_value,
			prepare_data_pointer p_data
		) {
			p_data->m_late.m_functions.append(
				new tokens::imp_token_put_literal(p_value)
			);
		}

	} // ns

} // ns