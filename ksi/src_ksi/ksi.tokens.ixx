module;

#include "../src/pre.h"

export module ksi.tokens;

export import ksi.ast;

export namespace ksi {

	using namespace just::text_literals;

	namespace tokens {

		struct token_base :
			public just::node_list<token_base>,
			public just::bases::with_deleter<token_base *>
		{
			virtual ~token_base() = default;

			virtual t_text_value name() const = 0;
			virtual void perform(prepare_data::pointer p_data) {}
		};

		struct nest_tokens {
			using pointer = nest_tokens *;
			using t_tokens = just::list<token_base, just::closers::compound_call_deleter<false>::template t_closer>;

			static void put_literal_prop_default(pointer p_nest, const var::any_var & p_value);

			using t_put_literal = decltype(&put_literal_prop_default);

			// data
			t_tokens		m_cats, m_types, m_functions;
			t_put_literal	m_fn_put_literal = &put_literal_prop_default;

			void splice(nest_tokens & p_other) {
				m_cats		.splice(p_other.m_cats);
				m_types		.splice(p_other.m_types);
				m_functions	.splice(p_other.m_functions);
			}

			void perform(prepare_data::pointer p_data) {
				m_cats.m_zero.node_apply_to_others([p_data](t_tokens::t_node_pointer p_node){
					p_node->node_target()->perform(p_data);
				});
				m_types.m_zero.node_apply_to_others([p_data](t_tokens::t_node_pointer p_node){
					p_node->node_target()->perform(p_data);
				});
				m_functions.m_zero.node_apply_to_others([p_data](t_tokens::t_node_pointer p_node){
					p_node->node_target()->perform(p_data);
				});
			}

			void put_literal(const var::any_var & p_value) {
				m_fn_put_literal(this, p_value);
			}
		};

		/*struct token_base_pos :
			public token_base
		{
			// data
			log_pos		m_log_pos;

			token_base_pos(const log_pos & p_log_pos) : m_log_pos{p_log_pos} {}
		};*/

		struct token_module_name :
			public token_base
		{
			// data
			t_text_value	m_name;

			token_module_name(const t_text_value & p_name) : m_name{p_name} {}

			t_text_value name() const override { return "token_module_name"_jt; }

			void perform(prepare_data::pointer p_data) override {
				p_data->m_ext_module_current = p_data->ext_module_open(m_name);
			}
		};

		struct token_type_add :
			public token_base
		{
			// data
			log_pos			m_log_pos;
			t_text_value	m_name;
			bool			m_is_local;

			token_type_add(const log_pos & p_log_pos, const t_text_value & p_name, bool p_is_local) :
				m_log_pos{p_log_pos}, m_name{p_name}, m_is_local{p_is_local}
			{}

			t_text_value name() const override { return "token_type_add"_jt; }

			void perform(prepare_data::pointer p_data) override {
				p_data->m_type_name = m_name;
				p_data->m_type_pos = m_log_pos;
				p_data->m_type_is_local = m_is_local;
				p_data->m_type_extends.clear();
			}
		};

		struct token_type_add_base :
			public token_base
		{
			// data
			type_extend_info	m_type_extend;

			token_type_add_base(const type_extend_info & p_type_extend) : m_type_extend{p_type_extend} {}

			t_text_value name() const override { return "token_type_add_base"_jt; }

			void perform(prepare_data::pointer p_data) override {
				p_data->m_type_extends.push_back(m_type_extend);
			}
		};

		struct token_struct_add :
			public token_base
		{
			t_text_value name() const override { return "token_struct_add"_jt; }

			void perform(prepare_data::pointer p_data) override {
				if( ! p_data->struct_add() ) {
					t_text_value v_message = just::implode<t_text_value::type>(
						{"deduce error: Duplicate type name: ", p_data->m_type_name}
					);
					p_data->error(p_data->m_type_pos.message(v_message) );
				}
				var::type_struct_pointer v_struct = p_data->m_ext_module_current->struct_last();
				v_struct->init_base();
				for( type_extend_info & v_it : p_data->m_type_extends ) {
					if( var::type_pointer v_type_source = p_data->type_find(v_it) ) {
						p_data->m_error_count += v_struct->inherit_from(
							{p_data->m_type_pos.m_path, v_it.m_pos}, v_type_source, p_data->m_log
						);
					}
				}
			} // fn
		};

		struct token_struct_end :
			public token_base
		{
			t_text_value name() const override { return "token_struct_end"_jt; }

			void perform(prepare_data::pointer p_data) override {
				p_data->m_ext_module_current->struct_last()->m_static->init();
			}
		};

		struct token_struct_prop_name :
			public token_base
		{
			using pointer = token_struct_prop_name *;

			// data
			log_pos			m_log_pos;
			t_text_value	m_name;
			var::any_var	m_value;

			token_struct_prop_name(const log_pos & p_log_pos, const t_text_value & p_name) :
				m_log_pos{p_log_pos}, m_name{p_name}
			{}

			t_text_value name() const override { return "token_struct_prop_name"_jt; }

			void perform(prepare_data::pointer p_data) override {
				var::type_struct_pointer v_type_struct = p_data->m_ext_module_current->struct_last();
				if( ! v_type_struct->prop_add(m_name, m_value) ) {
					typename var::type_struct::t_props::t_find_result v_res = v_type_struct->m_props.find(m_name);
					var::type_pointer v_type = v_res.m_value->m_type_source;
					p_data->error(m_log_pos.message(just::implode<t_text_value::type>({
						"deduce error: Property \"", m_name, "\" is already defined in type: ", v_type->m_name_full
					}) ) );
					if( v_type != v_type_struct ) {
						p_data->m_log->add(v_type->m_log_pos.message(just::implode<t_text_value::type>(
							{"info: See definition of type: ", v_type->m_name_full}
						) ) );
					}
				}
			} // fn
		};

		void nest_tokens::put_literal_prop_default(nest_tokens::pointer p_nest, const var::any_var & p_value) {
			token_struct_prop_name::pointer v_prop_token = static_cast<token_struct_prop_name::pointer>(
				p_nest->m_types.m_zero.m_prev
			);
			v_prop_token->m_value = p_value;
		}

	} // ns

} // ns