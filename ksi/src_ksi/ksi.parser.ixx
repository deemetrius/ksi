module;

#include "../src/pre.h"

export module ksi.parser;

export import ksi.space;
export import ksi.log;
import :state;
import :basic;
import :alt;

export namespace ksi {

	namespace parser {

		struct rule_file : public rule_alt<
			t_eof,
			t_opt_space
		> {};

		bool parse(fs::path p_path, t_text p_file, log_base::pointer p_log) {
			state v_state{p_path, p_file->data(), &rule_file::parse};
			tokens::nest v_tokens;
			ast::prepare_data v_data{p_log};
			do {
				if( ! v_state.m_next_parse(v_state, v_tokens, v_data) ) { v_state.done(); }
			} while( ! v_state.m_done );
			return v_state.m_nice;
		}

	} // ns

} // ns