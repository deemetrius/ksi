module;

#include "../src/pre.h"

export module ksi;

export import ksi.ast;

export namespace ksi {

	using namespace just::text_literals;

	namespace tokens {

		struct token_base {
			virtual ~token_base() = default;

			virtual t_text_value name() const = 0;
			virtual void perform(prepare_data::pointer p_data) {}
		};

		struct token_base_pos :
			public token_base
		{
			// data
			log_pos		m_log_pos;

			token_base_pos(const log_pos & p_log_pos) : m_log_pos{p_log_pos} {}
		};

		struct token_type_add :
			public token_base_pos
		{
			// data
			t_text_value	m_name;

			token_type_add(const log_pos & p_log_pos, const t_text_value & p_name) :
				token_base_pos{p_log_pos}, m_name{p_name}
			{}

			t_text_value name() const override { return "token_type_add"_jt; }

			void perform(prepare_data::pointer p_data) override {
				p_data->m_type_name = m_name;
				p_data->m_type_pos = m_log_pos;
			}
		};

		struct token_struct_add :
			public token_base
		{
			t_text_value name() const override { return "token_struct_add"_jt; }

			void perform(prepare_data::pointer p_data) override {
				if( ! p_data->m_ext_module_current->struct_add(p_data->m_type_name, p_data->m_type_pos) ) {
					t_text_value v_message = just::implode<t_text_value::type>(
						{"deduce error: Duplicate type name: ", p_data->m_type_name}
					);
					p_data->error(p_data->m_type_pos.message(v_message) );
				}
			}
		};

	} // ns

} // ns