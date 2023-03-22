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
				static bool check_tab_or_new_line(position & p_pos, bool & p_is_new_line) {
					p_is_new_line = false;
					switch( *p_pos.m_pos ) {
					case L'\n':
						p_pos.next_line();
						p_is_new_line = true;
						return true;
					case L'\r':
						p_pos.next_line(
							p_pos.m_pos[1] == L'\n' ? 2 : 1
						);
						p_is_new_line = true;
						return true;
					case L'\t':
						p_pos.next_tab();
						return true;
					}
					return false;
				}

				static bool take_line_comment(position & p_pos) {
					if( p_pos.m_pos[1] != L'-' ) { return false; }
					p_pos.next(2);
					bool v_is_new_line;
					do {
						if( check_tab_or_new_line(p_pos, v_is_new_line) ) {
							if( v_is_new_line ) { break; }
							continue;
						}
						if( *p_pos.m_pos == L'\0' ) { break; }
						p_pos.next();
					} while( true );
					return true;
				}

				static bool take_multiline_comment(position & p_pos, t_integer & p_depth) {
					if( p_pos.m_pos[1] != L'*' ) { return false; }
					p_pos.next(2);
					p_depth = 1;
					bool v_is_new_line;
					do {
						if( check_tab_or_new_line(p_pos, v_is_new_line) ) { continue; }
						switch( *p_pos.m_pos ) {
						case L'*': 
							if( p_pos.m_pos[1] == L'/' ) {
								p_pos.next(2);
								--p_depth;
							} else {
								p_pos.next();
							}
							break;
						case L'/':
							if( p_pos.m_pos[1] == L'*' ) {
								p_pos.next(2);
								++p_depth;
							} else {
								p_pos.next();
							}
							break;
						case L'\0':
							return true;
						default:
							p_pos.next();
						}
					} while( p_depth > 0 );
					return true;
				}

				bool parse(state & p_state, tokens::nest & p_tokens, ast::prepare_data & p_data) {
					position v_pos = p_state.m_pos;
					bool v_continue = true, v_is_new_line;
					t_integer v_depth = 0;
					do {
						if( check_tab_or_new_line(v_pos, v_is_new_line) ) { continue; }
						if( *v_pos.m_pos == L' ' ) { v_pos.next(); continue; }
						if( *v_pos.m_pos == L'-' && take_line_comment(v_pos) ) { continue; }
						if( *v_pos.m_pos == L'/' && take_multiline_comment(v_pos, v_depth) ) { continue; }
						break;
					} while( true );
					if( p_state.m_pos.differs(v_pos) ) {
						p_state.m_pos = v_pos;
						if( v_depth > 0 ) {
							text_str v_msg = just::implode({
								L"parse warning: Unclosed multiline comment with depth = "s,
								std::to_wstring(v_depth)
							});
							p_data.m_log->add( p_state.message(v_msg) );
						}
						return true;
					}
					return false;
				}

				void action(state & p_state, tokens::nest & p_tokens, ast::prepare_data & p_data) {}
			}; // t_data
		}; // t_opt_space
	
	} // ns

} // ns