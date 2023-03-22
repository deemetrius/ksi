module;

#include "../src/pre.h"

export module ksi.ast;

export import ksi.log;
export import ksi.space;

export namespace ksi {

	namespace ast {

		struct t_module_extension : public just::with_deleter<t_module_extension> {
			using pointer = t_module_extension *;

			// data
			t_module::pointer
				m_module;
			space::t_mod_ptr
				m_new_module;

			t_module_extension(space::pointer p_space, t_text p_name, t_integer & p_id) {
				m_module = p_space->mod_find(p_name);
				if( m_module == nullptr ) {
					m_new_module = std::make_unique<t_module>(p_id++, p_name);
					m_module = m_new_module.get();
				}
			}
		};

		struct prepare_data {
			using t_mod_ptr = std::unique_ptr<t_module_extension, just::hold_deleter>;
			using t_mods = std::map<text_str, t_mod_ptr, std::ranges::less>;
			using t_mods_iterator = t_mods::iterator;
			using t_try_emplace = std::pair<t_mods::iterator, bool>;

			// data
			space::pointer
				m_space;
			log_base::pointer
				m_log;
			t_integer
				m_mod_id;
			t_mods
				m_mods;

			prepare_data(space::pointer p_space, log_base::pointer p_log) :
				m_space{p_space},
				m_log{p_log},
				m_mod_id{p_space->m_mod_id}
			{}

			t_module_extension::pointer mod_get(t_text p_name) {
				t_mods_iterator v_it = m_mods.find(*p_name);
				if( v_it == m_mods.end() ) {
					t_try_emplace v_try = m_mods.try_emplace(*p_name,
						std::make_unique<t_module_extension>(m_space, p_name, m_mod_id)
					);
					return v_try.first->second.get();
				}
				return v_it->second.get();
			}
		};

	} // ns

} // ns