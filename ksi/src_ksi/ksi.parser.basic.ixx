module;

#include "../src/pre.h"

export module ksi.parser:basic;

import :state;

namespace ksi {

	namespace parser {

		using namespace std::string_literals;

		struct t_eof {
			static inline t_text s_name = L"t_eof"s;

			static bool check(state & p_state) { return true; }

			struct t_data {
				bool parse(state & p_state, tokens::nest & p_tokens, ast::prepare_data & p_data) {
					return *p_state.m_pos.m_pos == L'\0';
				}

				void action(state & p_state, tokens::nest & p_tokens, ast::prepare_data & p_data) {
					p_state.done_nice();
				}
			};
		};

		struct t_opt_space {
			static inline t_text s_name = L"t_opt_space"s;

			static bool check(state & p_state) { return ! p_state.m_was_space; }

			struct t_data {
				void take_line_comment(position & p_pos, bool & p_continue) {
					if( p_pos.m_pos[1] != L'-' ) { p_continue = false; return; }
					p_pos.next(2);
					bool v_continue = true;
					do {
						switch( *p_pos.m_pos ) {
						case L'\n':
							p_pos.next_line();
							v_continue = false;
							break;
						case L'\r':
						{
							t_integer v_skip = 1;
							if( p_pos.m_pos[1] == L'\n' ) { ++v_skip; }
							p_pos.next_line(v_skip);
							v_continue = false;
						}
						break;
						case L'\0':
							v_continue = false;
							break;
						case L'\t':
							p_pos.next_tab();
							break;
						default:
							p_pos.next();
						}
					} while( v_continue );
				}

				bool parse(state & p_state, tokens::nest & p_tokens, ast::prepare_data & p_data) {
					position v_pos = p_state.m_pos;
					bool v_continue = true;
					do {
						switch( *v_pos.m_pos ) {
						case L' ':
							v_pos.next();
							break;
						case L'\t':
							v_pos.next_tab();
							break;
						case L'\n':
							v_pos.next_line();
							break;
						case L'\r':
						{
							t_integer v_skip = 1;
							if( v_pos.m_pos[1] == L'\n' ) { ++v_skip; }
							v_pos.next_line(v_skip);
						}
						break;
						case L'-':
							take_line_comment(v_pos, v_continue);
							break;
						default:
							v_continue = false;
						}
					} while( v_continue );
					if( p_state.m_pos.differs(v_pos) ) {
						p_state.m_pos = v_pos;
						return true;
					}
					return false;
				}

				void action(state & p_state, tokens::nest & p_tokens, ast::prepare_data & p_data) {}
			}; // t_data
		}; // t_opt_space
	
	} // ns

} // ns