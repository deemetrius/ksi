module;

#include "../src/pre.h"

export module ksi.space;

export import just.hive;
export import ksi.act;
export import <memory>;

export namespace ksi {

	using namespace std::string_literals;

	struct t_module : public var::with_id_name, public just::with_deleter<t_module> {
		using pointer = t_module *;
		using t_vars = var::with_ring::o_hive<text_str, var::value, var::value_less>;
		using t_seqs = std::vector<act::sequence>;

		// data
		var::owner
			m_owner;
		t_vars
			m_vars;
		t_seqs
			m_seqs;
		act::sequence::pointer
			m_do;

		t_module(t_integer p_id, t_text p_name) : with_id_name{p_id, p_name}, m_vars{&m_owner} {
			m_do = &m_seqs.emplace_back();
		}
	};

	struct space {
		using pointer = space *;
		using t_mod_ptr = std::unique_ptr<t_module, just::hold_deleter>;
		using t_mods = just::hive<text_str, t_mod_ptr, std::ranges::less>;

		// data
		t_integer
			m_mod_id = n_id_mod;
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

		act::sequence::pointer seq_get(act::pos_seq p_seq_pos) {
			return &mod_get(p_seq_pos.m_module_id)->m_seqs[p_seq_pos.m_seq_id];
		}
	};

} // ns