module;

#include "../src/pre.h"

export module ksi.config;

export import <array>;
export import ksi.space;

export namespace ksi {

	using namespace just::text_literals;

	namespace var {
	
		struct config {
			using t_types = std::array<type_pointer, 7>;

			// data
			fs::path		m_path;
			log_list		m_log_system;
			log_pointer		m_log = &m_log_system;
			output_pointer	m_out = &just::g_console;
			bool			m_init = false;
			module_space	m_mod_ksi;
			module_space	m_mod_hidden;
			//
			type_null		m_null;
			type_link		m_link;
			type_ref		m_ref;
			type_type		m_type;
			type_bool		m_bool;
			type_int		m_int;
			type_float		m_float;
			type_text		m_text;
			type_array		m_array;
			//
			t_types			m_types;
			any_var			m_zero_var;

			config() :
				m_mod_ksi{"@ksi#"_jt},
				m_mod_hidden{"@hidden#"_jt},
				m_null	{&m_mod_ksi},
				m_link	{&m_mod_hidden},
				m_ref	{&m_mod_hidden},
				m_type	{&m_mod_ksi},
				m_bool	{&m_mod_ksi},
				m_int	{&m_mod_ksi},
				m_float	{&m_mod_ksi},
				m_text	{&m_mod_ksi},
				m_array	{&m_mod_ksi},
				m_types{&m_null, &m_type, &m_bool, &m_int, &m_float, &m_text, &m_array},
				m_zero_var(nullptr, &m_null)
			{
				m_mod_ksi.m_deleter = &just::closers::simple_none<module_space *>::close;
				m_mod_hidden.m_deleter = &just::closers::simple_none<module_space *>::close;
				//
				for( type_pointer v_type : m_types ) { type_reg(v_type); }
			}

			static void type_reg(type_pointer p_type) {
				p_type->m_is_added = p_type->m_module->type_reg(p_type);
			}

			void init() {
				if( m_init ) { return; }
				else { m_init = true; }
				m_null	.init();
				m_link	.init();
				m_ref	.init();
				m_type	.init();
				m_bool	.init();
				m_int	.init();
				m_float	.init();
				m_text	.init();
				m_array	.init();
			}

			static config * instance() {
				static config v_inst;
				return &v_inst;
			}
		};

		using config_pointer = config *;
		config_pointer g_config = nullptr;

		struct log_switcher {
			// data
			log_pointer		m_prev_log;

			log_switcher(log_pointer p_log) {
				m_prev_log = g_config->m_log;
				g_config->m_log = p_log;
			}

			~log_switcher() {
				g_config->m_log = m_prev_log;
			}
		};
	
	} // ns

} // ns