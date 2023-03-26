module;

#include "../src/pre.h"

export module ksi.parser:mod;

export import :basic;

namespace ksi {

	namespace parser {

		using namespace std::string_literals;

		struct t_module_def {
			static inline t_text s_name = L"t_module_def"s;

			static bool check(state & p_state) { return true; }

			struct t_data : public is_prefix_name<L'@'> {
				void action(state & p_state, tokens::nest & p_tokens, ast::prepare_data & p_data) {
					p_tokens.add(this);
				}

				void perform(ast::prepare_data & p_data) {
					p_data.m_mod_current = p_data.mod_get(this->m_name);
				}
			};
		};

	} // ns

} // ns