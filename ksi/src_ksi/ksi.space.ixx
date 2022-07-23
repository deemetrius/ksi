module;

#include "../src/pre.h"

export module ksi.space;

export import ksi.function;

export namespace ksi {

	struct module_base :
		public is_module
	{
		using pointer = module_base *;

		using t_cats_list = just::list<var::category, just::closers::compound_call_deleter<false>::template t_closer>;
		using t_cats_map = std::map<just::text, var::category::pointer, just::text_less>;
		using t_cats_insert = std::pair<t_cats_map::iterator, bool>;

		using t_structs = just::list<var::type_struct, just::closers::compound_call_deleter<false>::template t_closer>;
		using t_types = std::map<just::text, var::type_pointer, just::text_less>;
		using t_types_insert = std::pair<t_types::iterator, bool>;
		using t_types_used = just::hive<just::text, var::type_pointer, just::text_less>;

		// data
		t_cats_list		m_cats_list;
		t_cats_map		m_cats_map;
		t_structs		m_structs;
		t_types			m_types;
		t_types_used	m_types_used;

		static pointer cast(module_pointer p_module) { return static_cast<pointer>(p_module); }

		bool category_reg(var::category::pointer p_cat) {
			typename t_cats_insert v_res = m_cats_map.try_emplace(p_cat->m_name, p_cat);
			return v_res.second;
		}

		var::category::pointer category_find(const t_text_value & p_name) {
			typename t_cats_map::iterator v_it = m_cats_map.find(p_name);
			return (v_it == m_cats_map.end() ) ? nullptr : (*v_it).second;
		}

		bool type_reg(var::type_pointer p_type) {
			typename t_types_insert v_res = m_types.try_emplace(p_type->m_name, p_type);
			return v_res.second;
		}

		var::type_pointer type_find(const t_text_value & p_name) {
			typename t_types::iterator v_it = m_types.find(p_name);
			return (v_it == m_types.end() ) ? nullptr : (*v_it).second;
		}
	};

	struct module_space :
		public module_base,
		public just::node_list<module_space>,
		public just::bases::with_deleter<module_space *>
	{
		using pointer = module_space *;

		// data
		t_text_value	m_name;

		module_space(const t_text_value & p_name) : m_name{p_name} {}

		t_text_value name() const override { return m_name; }
	};

	struct space_data {
		// data
		t_integer	m_id_struct		= var::n_id_struct;
		t_integer	m_id_cat_custom	= var::n_id_cat_custom;
	};

	struct space_base {
		using t_modules_list = just::list<module_space, just::closers::compound_call_deleter<false>::template t_closer>;
		using t_modules_map = std::map<t_text_value, module_space::pointer, just::text_less>;

		// data
		t_modules_list	m_modules_list;
		t_modules_map	m_modules_map;
		space_data		m_data;
	};

	struct space :
		public space_base
	{
		using t_files = std::set<fs::path>;

		// data
		t_files					m_files;
		module_space::pointer	m_module_global;

		space();

		module_space::pointer module_find(const t_text_value & p_name) {
			typename t_modules_map::iterator v_it = m_modules_map.find(p_name);
			return v_it == m_modules_map.end() ? nullptr : (*v_it).second;
		}

		bool type_reg(var::type_pointer p_type) {
			bool ret = p_type->m_is_added ? true : (
				p_type->m_is_added = module_base::cast(p_type->m_module)->type_reg(p_type)
			);
			if( ! p_type->m_is_local ) {
				p_type->m_is_global = m_module_global->type_reg(p_type);
			}
			return ret;
		}
	};

} // ns