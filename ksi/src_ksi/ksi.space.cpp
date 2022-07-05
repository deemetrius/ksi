module ksi.space;

#include "../src/pre.h"

import ksi.config;

namespace ksi {

	using namespace just::text_literals;

	space::space() {
		m_modules_map.maybe_emplace(var::g_config->m_mod_ksi.m_name, &var::g_config->m_mod_ksi);
		m_mod_global = new t_module{"@global#"_jt};
		m_modules_list.m_zero.m_prev->node_attach(m_mod_global);
		m_modules_map.maybe_emplace(m_mod_global->m_name, m_mod_global);
	}

} // ns