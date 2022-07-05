module;

#include "../src/pre.h"

export module ksi.config;

export import ksi.space;

export namespace ksi {

	namespace var {
	
		struct config {
			// data
			fs::path		m_path;
			log_list		m_log_system;
			log_pointer		m_log = &m_log_system;
			output_pointer	m_out = &just::g_console;
			bool			m_wrong_key_notice = false;
			bool			m_init = false;
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
			any_var			m_zero_var;

			config() : m_zero_var(nullptr, &m_null) {}

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