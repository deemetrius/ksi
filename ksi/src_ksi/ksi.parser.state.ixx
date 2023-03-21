module;

#include "../src/pre.h"

export module ksi.parser:state;

import ksi.log;

namespace ksi {

	namespace tokens {

		struct nest {};

	} // ns

	namespace ast {

		struct prepare_data {
			// data
			log_base::pointer
				m_log;
		};

	} // ns

	namespace parser {

		struct state;

		using fn_parse = bool (*) (state & p_state, tokens::nest & p_tokens, ast::prepare_data & p_data);

		struct position {
			// data
			tc_char_pointer
				m_pos,
				m_pos_line = m_pos;
			t_integer
				m_tab_size = 4,
				m_line = 1,
				m_chars_add = 0;

			t_integer column() const { return m_pos - m_pos_line + 1 + m_chars_add; }
			t_pos pos() const { return {m_line, column()}; }

			void next(t_integer p_skip = 1) { m_pos += p_skip; }
			void next_tab() {
				if( t_integer v_delta = column() % m_tab_size; v_delta > 0 ) {
					m_chars_add += m_tab_size - v_delta;
				}
				++m_pos;
			}
			void next_line(t_integer p_skip = 1) {
				++m_line;
				m_pos += p_skip;
				m_pos_line = m_pos;
				m_chars_add = 0;
			}

			bool differs(const position & p_pos) const { return m_pos != p_pos.m_pos; }
		};

		struct state {
			// data
			fs::path
				m_path;
			position
				m_pos;
			fn_parse
				m_next_parse;
			bool
				m_was_space = false,
				m_done = false,
				m_nice = false;

			state(fs::path p_path, tc_char_pointer p_pos, fn_parse p_parse) :
				m_path{p_path},
				m_pos{p_pos},
				m_next_parse{p_parse}
			{}

			void done() { m_done = true; }
			void done_nice() { m_done = true; m_nice = true; }

			log_message message(t_text p_msg) { return {m_path, m_pos.pos(), p_msg}; }
		};

	} // ns

} // ns