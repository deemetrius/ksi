module;

#include "../src/pre.h"

export module ksi.space;

export import ksi.var;
export import <memory>;

export namespace ksi {

	struct t_module : public var::with_id_name, public just::with_deleter<t_module> {
		using pointer = t_module *;
		using t_vars = var::with_ring::o_hive<text_str, var::value, var::value_less>;
		using t_owner = var::with_ring::owner;

		// data
		t_owner
			m_owner;
		t_vars
			m_vars;

		t_module(t_integer p_id, t_text p_name) : with_id_name{p_id, p_name}, m_vars{&m_owner} {}
	};

	struct space {
		using pointer = space *;
		using t_mod_ptr = std::unique_ptr<t_module, just::hold_deleter>;
		using t_mods = std::map<text_str, t_mod_ptr, std::ranges::less>;
		using t_try_emplace = std::pair<t_mods::iterator, bool>;

		// data
		t_integer
			m_mod_id	= n_id_mod;
		t_mods
			m_mods;

		t_module::pointer mod_add(t_text p_name) {
			typename t_mods::iterator v_it = m_mods.find(*p_name);
			if( v_it == m_mods.end() ) {
				t_try_emplace v_try = m_mods.try_emplace(*p_name,
					std::make_unique<t_module>(m_mod_id, p_name)
				);
				++m_mod_id;
				return v_try.first->second.get();
			}
			return v_it->second.get();
		}

		t_module::pointer mod_find(t_text p_name) {
			typename t_mods::iterator v_it = m_mods.find(*p_name);
			return v_it == m_mods.end() ? nullptr : v_it->second.get();
		}
	};

} // ns