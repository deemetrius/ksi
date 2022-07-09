module;

#include "../src/pre.h"

export module ksi.space;

export import ksi.function;

export namespace ksi {

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

		//var::type_pointer get_type(const t_text_value & p_name) {}
	};

	struct space_base {
		using t_modules_list = just::list<module_space, just::closers::compound_call_deleter<false>::template t_closer>;
		using t_modules_map = std::map<t_text_value, module_space::pointer, just::text_less>;

		// data
		t_modules_list	m_modules_list;
		t_modules_map	m_modules_map;
	};

	struct space :
		public space_base
	{
		using t_files = std::set<fs::path>;

		// data
		t_files					m_files;
		module_space::pointer	m_mod_global;

		space();

		module_space::pointer get_module(const t_text_value & p_name) {
			typename t_modules_map::iterator v_it = m_modules_map.find(p_name);
			return v_it == m_modules_map.end() ? nullptr : (*v_it).second;
		}

		bool type_reg(var::type_pointer p_type) {
			bool ret = p_type->m_is_added ? true : (p_type->m_is_added = p_type->m_module->type_reg(p_type) );
			if( ! p_type->m_is_local ) {
				p_type->m_is_global = m_mod_global->type_reg(p_type);
			}
			return ret;
		}
	};

} // ns