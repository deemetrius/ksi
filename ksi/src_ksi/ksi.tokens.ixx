module;

#include "../src/pre.h"

export module ksi.tokens;

export import ksi.prepare_data;
import ksi.instructions;

export namespace ksi { namespace tokens {

	using namespace just::text_literals;
	using namespace std::literals::string_view_literals;

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

	//

	struct token_category_add :
		public token_base
	{
		// data
		var::creation_args	m_args;

		token_category_add(const var::creation_args & p_args) : m_args{p_args} {}

		t_text_value name() const override { return "token_category_add"_jt; }

		void perform(prepare_data::pointer p_data) override {
			if( ! p_data->category_add(m_args) ) {
				t_text_value v_message = just::implode<t_char>(
					{"deduce error: Duplicate category name: "sv, m_args.m_name}
				);
				p_data->error(m_args.m_log_pos.message(v_message) );
			}
		}
	};

	struct token_category_add_base :
		public token_base
	{
		// data
		fs::path		m_path;
		entity_info		m_extend;

		token_category_add_base(const fs::path & p_path, const entity_info & p_extend) :
			m_path(p_path), m_extend{p_extend} {}

		t_text_value name() const override { return "token_category_add_base"_jt; }

		void perform(prepare_data::pointer p_data) override {
			var::category::pointer v_cat_base = p_data->category_find(m_path, m_extend);
			if( v_cat_base ) {
				var::category::pointer v_cat_target = p_data->m_ext_module_current->category_last();
				if( v_cat_base == v_cat_target ) {
					t_text_value v_message = just::implode<t_char>(
						{"deduce error: Category could not include itself: "sv, v_cat_target->m_name_full}
					);
					p_data->error({m_path, v_message, m_extend.m_pos});
					return;
				}
				if( ! v_cat_target->m_includes.add(v_cat_base) ) {
					t_text_value v_message = just::implode<t_char>({
						"deduce error: Category "sv, v_cat_base->m_name_full,
						" was already included to: "sv, v_cat_target->m_name_full
					});
					p_data->error({m_path, v_message, m_extend.m_pos});
				}
			}
		} // fn
	};

	//

	struct token_type_add :
		public token_base
	{
		// data
		var::creation_args	m_args;

		token_type_add(const var::creation_args & p_args) : m_args{p_args} {}

		t_text_value name() const override { return "token_type_add"_jt; }

		void perform(prepare_data::pointer p_data) override {
			p_data->m_type_args = m_args;
			p_data->m_type_extends.clear();
			p_data->m_type_refers.clear();
		}
	};

	struct token_type_add_category :
		public token_base
	{
		t_text_value name() const override { return "token_type_add_category"_jt; }

		// data
		entity_info		m_refer;

		token_type_add_category(const entity_info & p_refer) : m_refer{p_refer} {}

		void perform(prepare_data::pointer p_data) override {
			p_data->m_type_refers.push_back(m_refer);
		}
	};

	struct late_token_refers :
		public token_base
	{
		t_text_value name() const override { return "late_token_refers"_jt; }

		// data
		fs::path						m_path;
		var::type_pointer				m_type;
		prepare_data::t_entity_items	m_type_refers;

		late_token_refers(const fs::path & p_path, var::type_pointer p_type,
			prepare_data::t_entity_items && p_type_refers
		) : m_path{p_path}, m_type{p_type}, m_type_refers{std::move(p_type_refers)} {}

		void perform(prepare_data::pointer p_data) override {
			for( entity_info & v_it : m_type_refers ) {
				if( var::category::pointer v_cat = p_data->category_find(m_path, v_it) ) {
					m_type->m_categories.add(v_cat);
				}
			}
		}
	};

	struct token_type_add_base :
		public token_base
	{
		// data
		entity_info		m_extend;

		token_type_add_base(const entity_info & p_extend) : m_extend{p_extend} {}

		t_text_value name() const override { return "token_type_add_base"_jt; }

		void perform(prepare_data::pointer p_data) override {
			p_data->m_type_extends.push_back(m_extend);
		}
	};

	struct late_token_extends :
		public token_base
	{
		t_text_value name() const override { return "late_token_extends"_jt; }

		// data
		var::extender::pointer	m_type;

		late_token_extends(var::extender::pointer p_type) : m_type(p_type) {}

		void perform(prepare_data::pointer p_data) override {
			var::type_pointer v_type = m_type->type();
			for( var::extender::t_bases_iter v_it : m_type->m_bases ) {
				v_type->m_categories.add_from( v_it->m_value->type()->m_categories );
			}
		}
	};

	struct late_token_type_init_categories :
		public token_base
	{
		t_text_value name() const override { return "late_token_type_init_categories"_jt; }

		// data
		var::type_pointer	m_type;

		late_token_type_init_categories(var::type_pointer p_type) : m_type{p_type} {}

		void perform(prepare_data::pointer p_data) override {
			m_type->init_categories();
		}
	};

	struct token_struct_add :
		public token_base
	{
		t_text_value name() const override { return "token_struct_add"_jt; }

		// data
		fs::path	m_path;

		token_struct_add(const fs::path & p_path) : m_path{p_path} {}

		void perform(prepare_data::pointer p_data) override {
			if( ! p_data->struct_add() ) {
				t_text_value v_message = just::implode<t_char>(
					{"deduce error: Duplicate type name: "sv, p_data->m_type_args.m_name}
				);
				p_data->error(p_data->m_type_args.m_log_pos.message(v_message) );
			}
			var::type_struct_pointer v_struct = p_data->m_ext_module_current->struct_last();
			v_struct->init_start();
			bool v_need_late_cats = false;
			if( p_data->m_type_refers.size() ) {
				v_need_late_cats = true;
				p_data->m_late.m_types.append(
					new tokens::late_token_refers(p_data->m_type_args.m_log_pos.m_path, v_struct,
						std::move(p_data->m_type_refers)
					)
				);
			}
			if( p_data->m_type_extends.size() ) {
				v_need_late_cats = true;
				for( entity_info & v_it : p_data->m_type_extends ) {
					if( var::type_pointer v_type_source = p_data->type_find(m_path, v_it) ) {
						p_data->m_error_count += v_struct->inherit_from(
							{p_data->m_type_args.m_log_pos.m_path, v_it.m_pos}, v_type_source, p_data->m_log
						);
					}
				}
				p_data->m_late.m_types.append(
					new tokens::late_token_extends(v_struct)
				);
			}
			if( v_need_late_cats ) {
				p_data->m_late.m_types.append(
					new tokens::late_token_type_init_categories(v_struct)
				);
			} else {
				v_struct->init_categories();
			}
		} // fn
	};

	struct token_struct_end :
		public token_base
	{
		t_text_value name() const override { return "token_struct_end"_jt; }

		void perform(prepare_data::pointer p_data) override {
			p_data->m_ext_module_current->struct_last()->init_end();
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
				p_data->error(m_log_pos.message(just::implode<t_char>({
					"deduce error: Property \""sv, m_name, "\" is already defined in type: "sv, v_type->m_name_full
				}) ) );
				if( v_type != v_type_struct ) {
					p_data->m_log->add(v_type->m_log_pos.message(just::implode<t_char>(
						{"info: See definition of type: "sv, v_type->m_name_full}
					) ) );
				}
			}
		} // fn
	};

	//

	struct late_token_function_body_add_base :
		public token_base
	{
		using pointer = late_token_function_body_add_base *;

		// data
		module_extension::pointer	m_ext_module;
		log_pos						m_log_pos;
		function::pointer			m_local = nullptr;
		function::pointer			m_global = nullptr;

		static void make_body(prepare_data::pointer p_data, function_body_user::pointer p_body) {
			p_data->m_body = std::make_unique<ast::body>(p_body);
		}
	};

	struct late_token_function_body_add_over_common :
		public late_token_function_body_add_base
	{
		t_text_value name() const override { return "late_token_function_body_add_over_common"_jt; }

		late_token_function_body_add_over_common() = default;

		void perform(prepare_data::pointer p_data) override {
			p_data->m_ext_module_current = m_ext_module;
			function_body_user::pointer v_body = m_ext_module->function_body_user_add(m_log_pos);
			p_data->m_over_common.append(new overloader<std::monostate>{m_local, m_global, std::monostate{}, v_body});
			//
			make_body(p_data, v_body);
		}
	};

	struct late_token_function_body_add_over_category :
		public late_token_function_body_add_base
	{
		t_text_value name() const override { return "late_token_function_body_add_over_category"_jt; }

		// data
		fs::path		m_path;
		entity_info		m_entity;

		late_token_function_body_add_over_category(fs::path p_path, const entity_info & p_entity) :
			m_path{p_path}, m_entity{p_entity}
		{}

		void perform(prepare_data::pointer p_data) override {
			p_data->m_ext_module_current = m_ext_module;
			function_body_user::pointer v_body = m_ext_module->function_body_user_add(m_log_pos);
			if( var::category::pointer v_key = p_data->category_find(m_path, m_entity) ) {
				p_data->m_over_category.append(new overloader<var::category::pointer>{
					m_local, m_global, v_key, v_body
				});
			}
			//
			make_body(p_data, v_body);
		}
	};

	struct late_token_function_body_add_over_type :
		public late_token_function_body_add_base
	{
		t_text_value name() const override { return "late_token_function_body_add_over_type"_jt; }

		// data
		fs::path		m_path;
		entity_info		m_entity;

		late_token_function_body_add_over_type(fs::path p_path, const entity_info & p_entity) :
			m_path{p_path}, m_entity{p_entity}
		{}

		void perform(prepare_data::pointer p_data) override {
			p_data->m_ext_module_current = m_ext_module;
			function_body_user::pointer v_body = m_ext_module->function_body_user_add(m_log_pos);
			if( var::type_pointer v_key = p_data->type_find(m_path, m_entity) ) {
				p_data->m_over_type.append(new overloader<var::type_pointer>{
					m_local, m_global, v_key, v_body
				});
			}
			//
			make_body(p_data, v_body);
		}
	};

	struct late_token_function_body_close :
		public token_base
	{
		t_text_value name() const override { return "late_token_function_body_close"_jt; }

		late_token_function_body_close() = default;

		void perform(prepare_data::pointer p_data) override {
			p_data->m_body->apply();
			p_data->m_body->m_fn->write(&just::g_console); // debug
			p_data->m_body.reset();
		}
	};

	struct token_function_add :
		public token_base
	{
		using pointer = token_function_add *;

		t_text_value name() const override { return "token_function_add"_jt; }

		// data
		late_token_function_body_add_base::pointer	m_late_token = nullptr;
		var::creation_args							m_args;

		token_function_add(const var::creation_args & p_args) : m_args{p_args} {}

		void perform(prepare_data::pointer p_data) override {
			prepare_data::t_fn_pair v_res = p_data->function_add(m_args);
			if( m_late_token ) {
				m_late_token->m_log_pos = m_args.m_log_pos;
				module_extension::pointer v_ext_module = p_data->m_ext_module_current;
				m_late_token->m_ext_module = v_ext_module;
				m_late_token->m_local = v_res.first;
				m_late_token->m_global = v_res.second;
			}
		}
	};

	struct late_token_function_add_arg :
		public token_base
	{
		t_text_value name() const override { return "late_token_function_add_arg"_jt; }

		// data
		t_text_value	m_name;

		late_token_function_add_arg(const t_text_value & p_name) : m_name{p_name} {}

		void perform(prepare_data::pointer p_data) override {
			function_body_user::pointer v_body = p_data->m_ext_module_current->function_body_user_last();
			v_body->arg_add(m_name);
		}
	};

	struct late_token_function_add_param :
		public token_base
	{
		using pointer = late_token_function_add_param *;

		t_text_value name() const override { return "late_token_function_add_param"_jt; }

		// data
		log_pos			m_log_pos;
		t_text_value	m_name;
		var::any_var	m_value;

		late_token_function_add_param(const log_pos & p_log_pos,
			const t_text_value & p_name//, const var::any_var & p_value = var::any_var{}
		) :
			m_log_pos{p_log_pos}, m_name{p_name}//, m_value{p_value}
		{}

		void perform(prepare_data::pointer p_data) override {
			function_body_user::pointer v_body = p_data->m_ext_module_current->function_body_user_last();
			if( ! v_body->arg_add(m_name, m_value) ) {
				t_text_value v_message = just::implode<t_char>(
					{"deduce error: Duplicate function param name: "sv, m_name}
				);
				p_data->error(m_log_pos.message(v_message) );
			}
		}
	};

	//

	struct late_token_plain_start :
		public token_base
	{
		using pointer = late_token_plain_start *;

		t_text_value name() const override { return "late_token_plain_start"_jt; }

		// data
		log_pos						m_log_pos;
		module_extension::pointer	m_ext_module;

		late_token_plain_start(const log_pos & p_log_pos) : m_log_pos{p_log_pos} {}

		void perform(prepare_data::pointer p_data) override {
			p_data->m_ext_module_current = m_ext_module;
			plain::pointer v_plain = p_data->plain_add(m_ext_module, m_log_pos);
			//
			late_token_function_body_add_base::make_body(p_data, &v_plain->m_fn_body);
		}
	};

	struct late_token_plain_end :
		public token_base
	{
		t_text_value name() const override { return "late_token_plain_end"_jt; }

		late_token_plain_end() = default;

		void perform(prepare_data::pointer p_data) override {
			p_data->m_body->apply();
			p_data->m_body->m_fn->write(&just::g_console); // debug
			p_data->m_body.reset();
		}
	};

	struct token_plain_start :
		public token_base
	{
		t_text_value name() const override { return "token_plain_start"_jt; }

		// data
		late_token_plain_start::pointer		m_late_token;

		token_plain_start(late_token_plain_start::pointer p_late_token) : m_late_token{p_late_token} {}

		void perform(prepare_data::pointer p_data) override {
			module_extension::pointer v_ext_module = p_data->m_ext_module_current;
			m_late_token->m_ext_module = v_ext_module;
		}
	};

	//

	struct imp_token_put_literal :
		public token_base
	{
		t_text_value name() const override { return "imp_token_put_literal"_jt; }

		// data
		var::any_var	m_value;

		imp_token_put_literal(const var::any_var & p_value) : m_value{p_value} {}

		void perform(prepare_data::pointer p_data) override {
			instr_type::const_pointer v_instr_type = &instructions::s_put_null;
			instr_data v_instr_data;
			if( m_value.m_type == &var::g_config->m_null ) { // $null#
			} else if( m_value.m_type == &var::g_config->m_all ) { // $all#
				v_instr_type = &instructions::s_put_all;
			} else if( m_value.m_type == &var::g_config->m_bool ) { // $bool#
				v_instr_type = &instructions::s_put_bool;
				v_instr_data.m_arg = static_cast<t_integer>(m_value.m_value.m_bool);
			} else if( m_value.m_type == &var::g_config->m_int ) { // $int#
				v_instr_type = &instructions::s_put_int;
				v_instr_data.m_arg = m_value.m_value.m_int;
			} else {
				v_instr_type = &instructions::s_put_literal;
				v_instr_data.m_param = p_data->literal_reg(m_value);
			}
			p_data->m_body->node_add(&ast::body::s_type_leaf, v_instr_type, v_instr_data);
		}
	};

	enum class how_put { copy, link, ref };

	struct imp_token_put_var :
		public token_base
	{
		t_text_value name() const override { return "imp_token_put_var"_jt; }
		
		// data
		how_put			m_how_put;
		t_text_value	m_name;

		imp_token_put_var(how_put p_how_put, const t_text_value & p_name) : m_how_put{p_how_put}, m_name{p_name} {}

		void perform(prepare_data::pointer p_data) override {
			instr_type::const_pointer v_instr_type = &instructions::s_put_var;
			switch( m_how_put ) {
			case how_put::link	: { v_instr_type = &instructions::s_put_var_link; break; }
			case how_put::ref	: { v_instr_type = &instructions::s_put_var_ref; break; }
			}
			instr_data v_instr_data;
			v_instr_data.m_param = p_data->m_body->m_fn->var_id(m_name);
			p_data->m_body->node_add(&ast::body::s_type_leaf, v_instr_type, v_instr_data);
		}
	};

	struct imp_token_add_op :
		public token_base
	{
		t_text_value name() const override { return "imp_token_add_op"_jt; }

		using operator_info_pointer = ast::body::operator_info::pointer;

		// data
		position				m_pos;
		operator_info_pointer	m_op;
		
		imp_token_add_op(const position & p_pos, operator_info_pointer p_op) : m_pos{p_pos}, m_op{p_op} {}

		void perform(prepare_data::pointer p_data) override {
			p_data->m_body->node_add_op(m_op, m_pos);
		}
	};

} } // ns ns