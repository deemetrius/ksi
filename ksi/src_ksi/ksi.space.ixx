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
	};

} // ns