module;

#include "../src/pre.h"

export module ksi.ast;

export import ksi.log;
export import ksi.space;
export import :tree;

export namespace ksi {

	namespace ast {

		struct t_module_extension : public just::with_deleter<t_module_extension> {
			using t_vars = std::map<text_str, t_index, std::ranges::less>;
			using t_vars_vec = std::vector<t_vars::iterator>;
			using t_vars_try = std::pair<t_vars::iterator, bool>;
			using pointer = t_module_extension *;

			// data
			t_module::pointer
				m_module;
			space::t_mod_ptr
				m_new_module;
			t_vars
				m_vars;
			t_vars_vec
				m_vars_vec;
			t_module::t_props
				m_props;

			t_module_extension(space::pointer p_space, t_text p_name, t_integer & p_id) {
				m_module = p_space->mod_find(p_name);
				if( m_module == nullptr ) {
					m_new_module = std::make_unique<t_module>(p_id++, p_name);
					m_module = m_new_module.get();
				}
			}

			void extend(space::pointer p_space, log_base::pointer p_log) {
				for( t_vars::iterator v_it : m_vars_vec ) {
					m_module->var_get(v_it->first);
				}
				m_vars.clear();
				m_vars_vec.clear();
				for( property_seq & v_it : m_props ) {
					m_module->m_props.emplace_back( std::move(v_it) );
				}
				m_props.clear();
				//
				if( m_new_module ) { p_space->mod_move(m_new_module, p_log); }
			}

			t_index inner_var_get(t_text p_name) {
				t_index ret = std::ssize(m_vars) + m_module->m_vars.ssize();
				t_vars_try v_try = m_vars.try_emplace(*p_name, ret);
				if( v_try.second ) {
					m_vars_vec.emplace_back(v_try.first);
				}
				return v_try.first->second;
			}

			t_index var_get(t_text p_name) {
				typename t_module::t_vars::iterator v_it = m_module->m_vars.find(*p_name);
				if( v_it == m_module->m_vars.end() ) {
					return inner_var_get(p_name);
				}
				return (*v_it).m_index;
			}
		};

		struct prepare_data {
			//using t_mod_ptr = std::unique_ptr<t_module_extension, just::hold_deleter>;
			using t_mods = std::map<text_str, t_module_extension, std::ranges::less>;
			using t_mods_iterator = t_mods::iterator;
			using t_try_emplace = std::pair<t_mods::iterator, bool>;
			using t_body_ptr = std::unique_ptr<body>;

			// data
			space::pointer
				m_space;
			log_base::pointer
				m_log;
			t_integer
				m_error_count = 0;
			t_integer
				m_mod_id;
			t_mods
				m_mods;
			t_module_extension::pointer
				m_mod_current;
			t_body_ptr
				m_body;

			prepare_data(space::pointer p_space, log_base::pointer p_log) :
				m_space{p_space},
				m_log{p_log},
				m_mod_id{p_space->m_mod_id}
			{}

			void extend() {
				for( typename t_mods::value_type & v_it : m_mods ) {
					v_it.second.extend(m_space, m_log);
				}
				m_space->m_mod_id = m_mod_id;
			}

			void log_add(log_message && p_msg) {
				++m_error_count;
				m_log->add( std::move(p_msg) );
			}

			t_module_extension::pointer mod_get(t_text p_name) {
				t_mods_iterator v_it = m_mods.find(*p_name);
				if( v_it == m_mods.end() ) {
					t_try_emplace v_try = m_mods.try_emplace(*p_name, m_space, p_name, m_mod_id);
					return &v_try.first->second;
				}
				return &v_it->second;
			}
		};

	} // ns

} // ns