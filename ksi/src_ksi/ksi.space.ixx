module;

#include "../src/pre.h"

export module ksi.space;

export import <set>;
export import ksi.function;

export namespace ksi {

	struct t_module :
		public module_base,
		public just::node_list<t_module>,
		public just::bases::with_deleter<t_module *>
	{
		using pointer = t_module *;

		// data
		t_text_value	m_name;

		t_module(const t_text_value & p_name) : m_name{p_name} {}

		t_text_value name() const override { return m_name; }
	};

	struct space_base {
		using t_modules_list = just::list<t_module, just::closers::compound_call_deleter<false>::template t_closer>;
		using t_modules_map = just::hive<t_text_value, t_module::pointer, just::text_less>;

		// data
		t_modules_list	m_modules_list;
		t_modules_map	m_modules_map;
	};

	struct space :
		public space_base
	{
		using t_files = std::set<fs::path>;

		// data
		t_files				m_files;
		t_module::pointer	m_mod_global;

		space();
	};

} // ns