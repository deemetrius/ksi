module ksi.space;

#include "../src/pre.h"

import ksi.config;

namespace ksi {

	using namespace just::text_literals;

	space::space() {
		m_modules_map.emplace(var::g_config->m_mod_ksi.m_name, &var::g_config->m_mod_ksi);
		m_mod_global = new module_space{"@global#"_jt};
		m_modules_list.append(m_mod_global);
		m_modules_map.emplace(m_mod_global->m_name, m_mod_global);
	}

} // ns