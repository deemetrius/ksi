module;

#include "../src/pre.h"

export module ksi.parser:basic;

export import :state;
import <cwctype>;
import <limits>;

namespace ksi {

	namespace parser {

		using namespace std::string_literals;

		struct t_eof {
			static inline t_text s_name = L"t_eof"s;
			static constexpr kind s_kind = kind::n_eof;

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
			static constexpr kind s_kind = kind::n_space;

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

		template <t_char ... C>
		struct is_char {
			t_char
				m_value = L'\0';
			t_pos
				m_pos;

			bool parse(state & p_state, tokens::nest & p_tokens, ast::prepare_data & p_data) {
				if( just::is_one_of(*p_state.m_pos.m_pos, C ...) ) {
					m_value = *p_state.m_pos.m_pos;
					m_pos = p_state.m_pos.pos();
					p_state.m_pos.next();
					return true;
				}
				return false;
			}
		};

		template <text_str::value_type C_char>
		struct is_prefix_name {
			// data
			t_text
				m_name;

			bool parse(state & p_state, tokens::nest & p_tokens, ast::prepare_data & p_data) {
				if( *p_state.m_pos.m_pos != C_char || ! std::iswalpha(p_state.m_pos.m_pos[1]) ) { return false; }
				position v_pos = p_state.m_pos;
				v_pos.next(2);
				while( std::iswalnum(*v_pos.m_pos) || *v_pos.m_pos == L'_' ) { v_pos.next(); }
				m_name = text_str{p_state.m_pos.m_pos, v_pos.m_pos};
				p_state.m_pos = v_pos;
				return true;
			}
		};

		struct is_name {
			// data
			t_text
				m_name;
			fs::path
				m_path;
			t_pos
				m_pos;

			bool parse(state & p_state, tokens::nest & p_tokens, ast::prepare_data & p_data) {
				if( ! std::iswalpha(*p_state.m_pos.m_pos) ) { return false; }
				position v_pos = p_state.m_pos;
				v_pos.next();
				while( std::iswalnum(*v_pos.m_pos) || *v_pos.m_pos == L'_' ) { v_pos.next(); }
				m_name = text_str{p_state.m_pos.m_pos, v_pos.m_pos};
				//
				m_path = p_state.m_path;
				m_pos = p_state.m_pos.pos();
				//
				p_state.m_pos = v_pos;
				return true;
			}
		};

		struct is_integer {
			using t_limits = std::numeric_limits<t_integer>;

			// data
			t_integer
				m_value = 0;
			t_pos
				m_pos;

			static bool is_sign(t_char p) { return just::is_one_of(p, L'-', L'+'); }
			static bool is_separator(t_char p) { return just::is_one_of(p, L'_', L'\''); }

			bool parse(state & p_state, tokens::nest & p_tokens, ast::prepare_data & p_data) {
				position v_pos = p_state.m_pos;
				if( is_sign(*v_pos.m_pos) ) {
					v_pos.next();
				}
				if( ! std::iswdigit(*v_pos.m_pos) ) { return false; }
				v_pos.next();
				while( std::iswdigit(*v_pos.m_pos) || is_separator(*v_pos.m_pos) ) { v_pos.next(); }
				if( ! p_state.m_pos.differs(v_pos) ) { return false; }
				text_str v_text{p_state.m_pos.m_pos, v_pos.m_pos};
				m_pos = p_state.m_pos.pos();
				p_state.m_pos = v_pos;
				std::erase_if(v_text, [](t_char p)->bool{
					return is_separator(p);
				});
				try {
					m_value = std::stoll(v_text);
				} catch( const std::out_of_range & ) {
					m_value = (v_text[0] == L'-') ? t_limits::min() : t_limits::max();
					text_str v_msg = just::implode({
						L"parse error: Integer literal is out of range ("s,
						std::to_wstring( v_text.size() ),
						L" characters)."s
					});
					p_data.log_add( p_state.message(v_msg) );
				}
				just::g_console << '{' << m_value << "}\n";
				return true;
			}
		};

	} // ns

} // ns