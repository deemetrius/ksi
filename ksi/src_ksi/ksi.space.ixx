module;

#include "../src/pre.h"

export module ksi.space;

export import just.hive;
export import just.ordered_map;
export import ksi.act;
export import <memory>;

export namespace ksi {

	using namespace std::string_literals;

	enum class property_status { n_undefined, n_calculating, n_ready };
	
	/*struct property_seq {
		// data
		property_status
			m_status = property_status::n_undefined;
		act::sequence
			m_seq;
	};*/

	namespace ast {

		struct ext_property {
			// data
			t_text
				m_name;
			act::sequence
				m_seq;

			//ext_property(t_text p_name, fs::path p_path) : m_name{p_name}, m_seq{p_path} {}
		};

	} // ns

	struct t_property {
		// data
		var::cell
			m_cell;
		t_index
			m_seq_index;
		property_status
			m_status = property_status::n_undefined;

		t_property(var::optr_nest::junction::pointer p_point, t_index p_seq_index) :
			m_cell{p_point},
			m_seq_index{p_seq_index}
		{}
	};

	struct t_module : public var::with_id_name<var::n_id_mod>, public just::with_deleter<t_module> {
		using pointer = t_module *;
		using t_seqs = std::vector<act::sequence>;
		using t_props = just::ordered_map<text_str, t_property>;

		// data
		var::junction
			m_point;
		t_seqs
			m_seqs;
		act::sequence::pointer
			m_do;
		t_props
			m_props;

		t_module(t_integer p_id, t_text p_name) : with_id_name{p_id, p_name} {
			m_do = &m_seqs.emplace_back(fs::path{});
		}

		t_index var_add(ast::ext_property & p_ext_prop) {
			t_index v_seq_position = std::ssize(m_seqs);
			m_seqs.emplace_back(std::move(p_ext_prop.m_seq));
			typename t_props::t_emplace_result v_it = m_props.try_emplace(*p_ext_prop.m_name, &m_point, v_seq_position);
			return v_it.first->second.m_index;
		}

		t_index var_find_id(t_text p_name) {
			typename t_props::t_map_iterator v_it = m_props.find(*p_name);
			return ( v_it == m_props.m_map.end() ) ? -1 : v_it->second.m_index;
		}
	};

	struct space {
		using pointer = space *;
		using t_mod_ptr = std::unique_ptr<t_module, just::hold_deleter>;
		using t_mods = just::hive<text_str, t_mod_ptr, std::ranges::less>;

		// data
		t_integer
			m_mod_id = var::n_id_mod;
		t_mods
			m_mods;
		t_module::pointer
			m_mod_main;

		space() {
			m_mod_main = mod_add(L"@main"s);
		}

		t_module::pointer mod_add(t_text p_name) {
			typename t_mods::iterator v_it = m_mods.find(*p_name);
			if( v_it == m_mods.end() ) {
				v_it = m_mods.try_emplace(*p_name,
					std::make_unique<t_module>(m_mod_id, p_name)
				);
				++m_mod_id;
				return (*v_it).m_value->get();
			}
			return (*v_it).m_value->get();
		}

		t_module::pointer mod_find(t_text p_name) {
			typename t_mods::iterator v_it = m_mods.find(*p_name);
			return v_it == m_mods.end() ? nullptr : (*v_it).m_value->get();
		}

		t_module::pointer mod_get(t_index p_index) {
			return m_mods.m_vec[p_index].get();
		}

		void mod_move(t_mod_ptr & p_mod_ptr, log_base::pointer p_log) {
			typename t_mods::iterator v_it = m_mods.try_emplace(*p_mod_ptr->m_name,
				std::move(p_mod_ptr)
			);
			if( typename t_mods::value_type v = *v_it; v.m_value->get()->m_id != (v.m_index + var::n_id_mod) ) {
				t_text v_msg = just::implode({
					L"extending module notice: Module id differs: "s,
					v.m_value->get()->m_name,
					L" (pos: "s,
					std::to_wstring(v.m_index),
					L", id: "s,
					std::to_wstring(v.m_value->get()->m_id - var::n_id_mod),
					L")"s
				});
				p_log->add({fs::path{}, {0,0}, v_msg});
			}
		}

		act::sequence::pointer seq_get(act::pos_module_aspect p_seq_pos) {
			return &mod_get(p_seq_pos.m_module_id)->m_seqs[p_seq_pos.m_aspect_id];
		}

		t_property & var_get(act::pos_module_aspect p_var_pos) {
			return mod_get(p_var_pos.m_module_id)->m_props[p_var_pos.m_aspect_id].second.m_value;
		}
	};

} // ns