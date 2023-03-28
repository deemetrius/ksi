module;

#include "../src/pre.h"

export module ksi.parser;

import :rules;

export namespace ksi {

	namespace parser {

		bool parse(space::pointer p_space, fs::path p_path, t_text p_file, log_base::pointer p_log) {
			state v_state{p_path, p_file->data(), &nest::rule_file::parse};
			tokens::nest v_tokens;
			ast::prepare_data v_data{p_space, p_log};
			//
			do {
				if( ! v_state.m_next_parse(v_state, v_tokens, v_data) ) { v_state.done(); }
			} while( ! v_state.m_done );
			//
			if( ! v_state.m_nice || v_data.m_error_count > 0 ) { return false; }
			v_tokens.go(v_data);
			//
			if( v_data.m_error_count > 0 ) { return false; }
			v_data.extend();
			//
			return true;
		}

	} // ns

} // ns