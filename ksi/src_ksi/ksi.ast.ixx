module;

#include "../src/pre.h"

export module ksi.ast;

export import ksi.log;
export import ksi.undefined_yet;
export import :tree;

export namespace ksi {

	namespace ast {

		using namespace std::string_literals;

		struct t_module_extension : public just::with_deleter<t_module_extension> {
			using t_props = just::ordered_map<text_str, ext_property>;
			using pointer = t_module_extension *;

			// data
			t_module::pointer
				m_module;
			space::t_mod_ptr
				m_new_module;
			t_props
				m_props;

			t_module_extension(space::pointer p_space, t_text p_name, t_integer & p_id) {
				m_module = p_space->mod_find(p_name);
				if( m_module == nullptr ) {
					m_new_module.set(std::in_place_type<t_module>, p_id++, p_name);
					m_module = m_new_module.get();
				}
			}

			void extend(space::pointer p_space, log_base::pointer p_log) {
				// vars
				for( typename t_props::t_map_value_type & v_it : m_props ) {
					m_module->var_add(v_it.second.m_value);
				}
				//
				if( !m_new_module.empty() ) { p_space->mod_move(m_new_module, p_log); }
			}

			t_index var_find_id(t_text p_name) {
				t_index v_index = m_module->var_find_id(p_name);
				if( v_index != -1 ) { return v_index; }
				typename t_props::t_map_iterator v_it = m_props.find(*p_name);
				return ( v_it == m_props.m_map.end() ) ? (-1) : ( v_it->second.m_index + m_module->m_props.ssize() );
			}

			t_props::t_map_iterator var_add(t_text p_name, fs::path p_path) {
				typename t_props::t_emplace_result v_it = m_props.try_emplace(*p_name, p_name, p_path);
				return v_it.first;
			}
		};

		struct prepare_data {
			using t_mods = std::map<text_str, t_module_extension, std::ranges::less>;
			using t_mods_iterator = t_mods::iterator;
			using t_mods_vec = std::vector<t_mods_iterator>;
			using t_try_emplace = std::pair<t_mods::iterator, bool>;
			using t_body_ptr = std::unique_ptr<body>;
			using t_map_prop_iterator = t_module_extension::t_props::t_map_iterator;

			// data
			space::pointer
				m_space;
			log_base::pointer
				m_log;
			t_integer
				m_error_count = 0;
			t_integer
				m_mod_id;
			space::t_action_pool_ptr
				m_action_pool;
			t_mods
				m_mods;
			t_mods_vec
				m_mods_vec;
			t_module_extension::pointer
				m_mod_current;
			t_body_ptr
				m_body;
			act::pos_module_aspect
				m_var_pos;

			prepare_data(space::pointer p_space, log_base::pointer p_log) :
				m_space{p_space},
				m_log{p_log},
				m_mod_id{p_space->m_mod_id},
				m_action_pool{std::make_unique<space::t_action_pool>()}
			{}

			void extend() {
				m_space->m_action_pools.push_front( std::move(m_action_pool) );
				//
				for( t_mods_iterator v_it : m_mods_vec ) {
					v_it->second.extend(m_space, m_log);
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
					m_mods_vec.emplace_back(v_try.first);
					return &v_try.first->second;
				}
				return &v_it->second;
			}

			t_index var_add(t_text p_name, fs::path p_path, t_pos p_pos) {
				t_index v_var_id = m_mod_current->var_find_id(p_name);
				if( v_var_id == -1 ) {
					v_var_id = m_mod_current->m_module->m_props.ssize() + m_mod_current->m_props.ssize();
				} else {
					t_text v_msg = just::implode({
						L"deduce notice: "s,
						m_mod_current->m_module->m_name,
						L" Variable is already defined: "s,
						p_name
					});
					m_log->add({p_path, p_pos, v_msg});
					t_map_prop_iterator v_it = m_mod_current->var_add(p_name, p_path);
					m_body = std::make_unique<body>( &v_it->second.m_value.m_seq );
				}
				t_map_prop_iterator v_it = m_mod_current->var_add(p_name, p_path);
				m_body = std::make_unique<body>( &v_it->second.m_value.m_seq );

				return v_var_id;
			}
		};

	} // ns

} // ns