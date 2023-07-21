module;

#include "../src/pre.h"

export module ksi.space;

export import just.ptr;
export import just.pool;
export import just.ordered_map;
export import ksi.act;
export import <forward_list>;

export namespace ksi {

	//using namespace std::string_literals;
	using namespace std::literals::string_view_literals;

	enum class property_status { n_undefined, n_calculating, n_ready };

	namespace ast {

		struct ext_property : public log_pos {
			// data
			t_text
				m_name;
			act::sequence
				m_seq;
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

		t_property(t_index p_seq_index) :
			m_cell{&var::optr_nest::s_root_junction},
			m_seq_index{p_seq_index}
		{}
	};

	struct t_module : public var::with_id_name<var::n_id_mod> {
		using pointer = t_module *;
		using t_seqs = std::vector<act::sequence>;
		using t_props = just::ordered_map<text_str, t_property>;

		// data
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
			typename t_props::t_emplace_result v_it = m_props.try_emplace(*p_ext_prop.m_name, v_seq_position);
			if( v_it.second ) {
				m_seqs.emplace_back( std::move(p_ext_prop.m_seq) );
			}
			return v_it.first->second.m_index;
		}

		t_index var_find_id(t_text p_name) {
			typename t_props::t_map_iterator v_it = m_props.find(*p_name);
			return ( v_it == m_props.m_map.end() ) ? -1 : v_it->second.m_index;
		}
	};

	struct space {
		using pointer = space *;
		using t_mod_ptr = just::ptr<t_module>;
		using t_mods = just::ordered_map<text_str, t_mod_ptr>;
		//
		using t_action_pool = just::pool<act::action, 16>;
		using t_action_pool_ptr = std::unique_ptr<t_action_pool>;
		using t_action_pools = std::forward_list<t_action_pool_ptr>;

		// data
		t_integer
			m_mod_id = var::n_id_mod;
		t_mods
			m_mods;
		t_module::pointer
			m_mod_main;
		t_action_pools
			m_action_pools;

		space() {
			m_mod_main = mod_add(L"@main"s);
		}

		t_module::pointer mod_add(t_text p_name) {
			typename t_mods::t_map_iterator v_it = m_mods.find(*p_name);
			if( v_it == m_mods.m_map.end() ) {
				typename t_mods::t_emplace_result v_add = m_mods.try_emplace(*p_name,
					std::in_place_type<t_module>, m_mod_id, p_name
				);
				++m_mod_id;
				return v_add.first->second.m_value.get();
			}
			return v_it->second.m_value.get();
		}

		t_module::pointer mod_find(t_text p_name) {
			typename t_mods::t_map_iterator v_it = m_mods.find(*p_name);
			return v_it == m_mods.m_map.end() ? nullptr : v_it->second.m_value.get();
		}

		t_module::pointer mod_get(t_index p_index) {
			return m_mods[p_index].second.m_value.get();
		}

		void mod_move(t_mod_ptr & p_mod_ptr, log_base::pointer p_log) {
			typename t_mods::t_emplace_result v_it = m_mods.try_emplace(*p_mod_ptr->m_name,
				std::move(p_mod_ptr)
			);
			if( typename t_mods::value_type & v = v_it.first->second; v.m_value->m_id != (v.m_index + var::n_id_mod) ) {
				text_str
					v_position = std::to_wstring(v.m_index),
					v_index = std::to_wstring(v.m_value->m_id - var::n_id_mod);
				t_text v_msg = just::implode({
					L"extending module notice: Module id differs: "sv,
					v.m_value->m_name.view(),
					L" (pos: "sv,
					static_cast<text_view>(v_position),
					L", id: "sv,
					static_cast<text_view>(v_index),
					L")"sv
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

	//

	just::output_base & operator << (just::output_base & p_out, t_module::pointer p_mod) {
		for( t_index v_pos = 0; act::sequence & v_it : p_mod->m_seqs ) {
			p_out << "seq " << v_pos << just::g_new_line;
			p_out << v_it;
			++v_pos;
		}
		return p_out;
	}

} // ns