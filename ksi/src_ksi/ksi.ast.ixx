module;

#include "../src/pre.h"

export module ksi.ast;

import <concepts>;
import just.files;
export import ksi.config;

export namespace ksi {

	using namespace just::text_literals;

	using file_read_result = just::result<just::text>;

	file_read_result file_read(const fs::path & p_path, log_base::pointer p_log, t_int_ptr & p_error_count) {
		just::file_read_status v_read_status;
		file_read_result v_ret{false, just::file_read(p_path, v_read_status)};
		switch( v_read_status ) {
		case just::file_read_status::fail_get_size :
			++p_error_count;
			p_log->add({p_path, "error: Unable to get file size."_jt});
			break;
		case just::file_read_status::fail_open :
			++p_error_count;
			p_log->add({p_path, "error: Unable to open file."_jt});
			break;
		case just::file_read_status::fail_read :
			++p_error_count;
			p_log->add({p_path, "error: Unable to read file."_jt});
			break;
		default:
			v_ret.m_nice = true;
		}
		return v_ret;
	}

	struct prepare_data;
	using prepare_data_pointer = prepare_data *;

	namespace tokens {

		struct token_base :
			public just::node_list<token_base>,
			public just::bases::with_deleter<token_base *>
		{
			virtual ~token_base() = default;

			virtual t_text_value name() const = 0;
			virtual void perform(prepare_data_pointer p_data) {}
		};

		struct nest_tokens {
			using pointer = nest_tokens *;
			using t_tokens = just::list<token_base, just::closers::compound_call_deleter<false>::template t_closer>;

			static void put_literal_prop_default(pointer p_nest, const var::any_var & p_value);

			using t_put_literal = decltype(&put_literal_prop_default);

			// data
			t_tokens		m_cats, m_types, m_functions;
			t_put_literal	m_fn_put_literal = &put_literal_prop_default;

			void clear() {
				m_cats.clear();
				m_types.clear();
				m_functions.clear();
			}

			void splice(nest_tokens & p_other) {
				m_cats		.splice(p_other.m_cats);
				m_types		.splice(p_other.m_types);
				m_functions	.splice(p_other.m_functions);
			}

			void perform(prepare_data_pointer p_data) {
				for( t_tokens::t_node_pointer p_node : m_cats ) {
					p_node->node_target()->perform(p_data);
				}
				for( t_tokens::t_node_pointer p_node : m_types ) {
					p_node->node_target()->perform(p_data);
				}
				for( t_tokens::t_node_pointer p_node : m_functions ) {
					p_node->node_target()->perform(p_data);
				}
			}

			void put_literal(const var::any_var & p_value) {
				m_fn_put_literal(this, p_value);
			}
		};

	} // ns

	struct module_extension :
		public module_base,
		public just::node_list<module_extension>,
		public just::bases::with_deleter<module_extension *>
	{
		using pointer = module_extension *;

		// data
		module_space::pointer	m_module;

		module_extension(module_space::pointer p_module) : m_module{p_module} {
			init();
		}

		t_text_value name() const override { return m_module->m_name; }

		//template <bool C_types>
		void init() {
			//if constexpr( C_types ) {
				m_cats_map = m_module->m_cats_map;
				m_types = m_module->m_types;
				m_functions_map = m_module->m_functions_map;
			//}
			/*for( typename t_types_used::t_info && v_it : m_module->m_types_used ) {
				m_types_used.maybe_emplace(v_it.key(), *v_it.m_value);
			}*/
		}

		void apply() {
			m_module->m_cats_map = m_cats_map;
			m_module->m_types = m_types;
			m_module->m_functions_map = m_functions_map;
			//std::ranges::swap(m_types_used, m_module->m_types_used);
			//m_types_used.clear();
			//init<false>();
			m_module->m_cats_list.splice(m_cats_list);
			m_module->m_structs.splice(m_structs);
			m_module->m_functions_list.splice(m_functions_list);
		}

		//

		bool category_add(t_integer & p_id, const var::creation_args & p_args) {
			var::category::pointer v_cat = new var::category(m_module, p_id, p_args);
			m_cats_list.append(v_cat);
			return v_cat->m_is_added = category_reg(v_cat);
		}

		var::category::pointer category_last() {
			return m_cats_list.m_zero.node_empty() ? nullptr : m_cats_list.m_zero.m_prev->node_target();
		}

		//

		bool struct_add(t_integer & p_id, const var::creation_args & p_args) {
			var::type_struct_pointer v_struct = new var::type_struct(m_module, p_id, p_args);
			m_structs.append(v_struct);
			return v_struct->m_is_added = type_reg(v_struct);
		}

		var::type_struct_pointer struct_last() {
			return m_structs.m_zero.node_empty() ? nullptr : m_structs.m_zero.m_prev->node_target();
		}

		//

		bool function_add(const var::creation_args & p_args) {
			function::pointer v_fn = new function(m_module, p_args.m_name);
			m_functions_list.append(v_fn);
			return v_fn->m_is_added = function_reg(v_fn);
		}

		function::pointer function_last() {
			return m_functions_list.m_zero.node_empty() ? nullptr : m_functions_list.m_zero.m_prev->node_target();
		}
	};

	enum class file_status {
		loaded,
		in_process,
		with_error,
		absent,
		unknown
	};

	struct entity_info {
		// data
		position					m_pos;
		t_text_value				m_name;
		t_text_value				m_module_name;
	};

	struct prepare_data :
		public space_base
	{
		using pointer = prepare_data *;
		using t_files = std::map<fs::path, file_status>;
		using t_space_pointer = space *;
		using t_ext_modules_map = std::map<t_text_value, module_extension::pointer, just::text_less>;
		using t_ext_modules_list = just::list<module_extension,
			just::closers::compound_call_deleter<false>::template t_closer
		>;
		using t_entity_items = std::vector<entity_info>;

		// data
		t_space_pointer				m_space;
		log_base::pointer			m_log;
		t_files						m_files;
		t_int_ptr					m_error_count = 0;
		t_ext_modules_list			m_ext_modules_list;
		t_ext_modules_map			m_ext_modules_map;
		module_extension::pointer	m_ext_module_current = nullptr;
		module_extension::pointer	m_ext_module_global = nullptr;
		var::creation_args			m_type_args;
		t_entity_items				m_type_extends;
		t_entity_items				m_type_refers;
		tokens::nest_tokens			m_late;

		prepare_data(t_space_pointer p_space, log_base::pointer p_log) : m_space{p_space}, m_log{p_log} {
			m_ext_module_global = ext_module_open("@global#"_jt);
			m_data = m_space->m_data;
		}

		void error(const log_message & p_message) {
			++m_error_count;
			m_log->add(p_message);
		}

		bool late() {
			if( m_error_count ) { return false; }
			m_late.perform(this);
			if( m_error_count == 0 ) {
				m_late.clear();
				return true;
			}
			return false;
		}

		module_extension::pointer ext_module_find(const t_text_value & p_module_name) {
			typename t_ext_modules_map::iterator v_it = m_ext_modules_map.find(p_module_name);
			return (v_it == m_ext_modules_map.end() ) ? nullptr : (*v_it).second;
		}

		bool category_add(const var::creation_args & p_args) {
			bool ret = m_ext_module_current->category_add(m_data.m_id_cat_custom, p_args);
			if( ! p_args.m_is_local ) {
				var::category::pointer v_cat = m_ext_module_current->category_last();
				v_cat->m_is_global = m_ext_module_global->category_reg(v_cat);
			}
			return ret;
		}

		bool struct_add() {
			bool ret = m_ext_module_current->struct_add(m_data.m_id_struct, m_type_args);
			if( ! m_type_args.m_is_local ) {
				var::type_struct_pointer v_struct = m_ext_module_current->struct_last();
				v_struct->m_is_global = m_ext_module_global->type_reg(v_struct);
			}
			return ret;
		}

		bool function_add(const var::creation_args & p_args) {
			bool ret = m_ext_module_current->function_add(p_args);
			if( ! p_args.m_is_local ) {
				m_ext_module_global->function_add(p_args);
			}
			return ret;
		}

		//

		var::category::pointer impl_category_find(
			const fs::path & p_path, module_base::pointer p_module, const entity_info & p_extend
		) {
			var::category::pointer v_ret = p_module->category_find(p_extend.m_name);
			if( v_ret == nullptr ) {
				error({p_path, just::implode<t_text_value::type>(
					{"deduce error: Category was not defined yet: ", p_extend.m_name, p_extend.m_module_name}
				), p_extend.m_pos});
			}
			return v_ret;
		}

		var::category::pointer category_find(const fs::path & p_path, const entity_info & p_extend) {
			module_extension::pointer v_ext_module = nullptr;
			switch( p_extend.m_module_name.size() ) {
			case 0:
				v_ext_module = m_ext_module_global;
				break;
			case 1:
				v_ext_module = m_ext_module_current;
				break;
			default:
				v_ext_module = ext_module_find(p_extend.m_module_name);
				if( v_ext_module == nullptr ) {
					module_space::pointer v_module = m_space->module_find(p_extend.m_module_name);
					if( v_module == nullptr ) {
						error({p_path, just::implode<t_text_value::type>(
							{"deduce error: Module was not defined yet: ", p_extend.m_module_name}
						), p_extend.m_pos});
						return nullptr;
					}
					return impl_category_find(p_path, v_module, p_extend);
				}
			}
			return impl_category_find(p_path, v_ext_module, p_extend);
		}

		//

		var::type_pointer impl_type_find(module_base::pointer p_module, const entity_info & p_extend) {
			var::type_pointer v_ret = p_module->type_find(p_extend.m_name);
			if( v_ret == nullptr ) {
				error({m_type_args.m_log_pos.m_path, just::implode<t_text_value::type>(
					{"deduce error: Type was not defined yet: ", p_extend.m_name, p_extend.m_module_name}
				), p_extend.m_pos});
			}
			return v_ret;
		}

		var::type_pointer type_find(const entity_info & p_extend) {
			module_extension::pointer v_ext_module = nullptr;
			switch( p_extend.m_module_name.size() ) {
			case 0:
				v_ext_module = m_ext_module_global;
				break;
			case 1:
				v_ext_module = m_ext_module_current;
				break;
			default:
				v_ext_module = ext_module_find(p_extend.m_module_name);
				if( v_ext_module == nullptr ) {
					module_space::pointer v_module = m_space->module_find(p_extend.m_module_name);
					if( v_module == nullptr ) {
						error({m_type_args.m_log_pos.m_path, just::implode<t_text_value::type>(
							{"deduce error: Module was not defined yet: ", p_extend.m_module_name}
						), p_extend.m_pos});
						return nullptr;
					}
					return impl_type_find(v_module, p_extend);
				}
			}
			return impl_type_find(v_ext_module, p_extend);
		}

		//

		module_space::pointer module_get(const t_text_value & p_name) {
			if( typename t_modules_map::iterator v_it = m_space->m_modules_map.find(p_name);
				v_it != m_space->m_modules_map.end()
			) {
				return (*v_it).second;
			}
			if( typename t_modules_map::iterator v_it = m_modules_map.find(p_name); v_it != m_modules_map.end() ) {
				return (*v_it).second;
			}
			module_space::pointer v_module = new module_space{p_name};
			m_modules_list.append(v_module);
			m_modules_map.try_emplace(p_name, v_module);
			return v_module;
		}

		module_extension::pointer ext_module_open(const t_text_value & p_name) {
			if( typename t_ext_modules_map::iterator v_it = m_ext_modules_map.find(p_name);
				v_it != m_ext_modules_map.end()
			) {
				return (*v_it).second;
			}
			module_extension::pointer v_ext_module = new module_extension{module_get(p_name)};
			m_ext_modules_list.append(v_ext_module);
			m_ext_modules_map.try_emplace(p_name, v_ext_module);
			return v_ext_module;
		}

		void apply() {
			if( m_error_count ) { return; }
			m_space->m_data = m_data;
			m_ext_modules_list.m_zero.node_apply_to_others([](t_ext_modules_list::t_node_pointer p_node){
				p_node->node_target()->apply();
			});
			m_modules_list.m_zero.node_apply_to_others([this](t_modules_list::t_node_pointer p_node){
				module_space::pointer v_module = p_node->node_target();
				m_space->m_modules_map.try_emplace(v_module->m_name, v_module);
			});
			m_space->m_modules_list.splice(m_modules_list);
			for( typename t_files::value_type & v_it : m_files ) {
				if( v_it.second == file_status::loaded ) {
					m_space->m_files.emplace(v_it.first);
				}
			}
		}

		file_status check_path(const fs::path & p_path) {
			if( m_space->m_files.contains(p_path) ) {
				return file_status::loaded;
			}
			t_files::iterator v_it = m_files.find(p_path);
			return v_it == m_files.end() ? file_status::unknown : (*v_it).second;
		}

		file_status load_folder(const fs::path & p_path);
		file_status load_file(const fs::path & p_path);

	private:
		file_status folder_fail(
			file_status p_status,
			const fs::path & p_path,
			const fs::path p_priority_path,
			const just::text & p_message,
			just::t_index p_line,
			just::t_index p_char
		) {
			error({ p_priority_path, p_message, {p_line, p_char} });
			m_files.insert_or_assign(p_priority_path, p_status);
			m_files.insert_or_assign(p_path, p_status);
			return p_status;
		}
	};

	bool is_wrong_char(char p_char) {
		return just::is_one_of(p_char, '\t', '"', ':', '/', '\\', '*', '?', '<', '>', '|');
	}

} // ns