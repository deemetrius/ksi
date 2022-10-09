module ksi.space;

#include "../src/pre.h"

import ksi.config;

namespace ksi {

	using namespace just::text_literals;

	space::space() : m_plain_pos{&m_plain_list.m_zero} {
		m_modules_map.try_emplace(var::g_config->m_mod_ksi.m_name, &var::g_config->m_mod_ksi);
		m_module_global = new module_space{"@global#"_jt, true};
		m_modules_list.append(m_module_global);
		m_modules_map.try_emplace(m_module_global->m_name, m_module_global);
		//
		for( var::type_pointer v_type : var::g_config->m_types ) {
			v_type->m_is_global = m_module_global->type_reg(v_type);
		}
	}

} // ns