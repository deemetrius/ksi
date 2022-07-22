module;

#include "../src/pre.h"

export module ksi.function;

export import <vector>;
export import just.text;
export import just.array;
export import just.hive;
export import ksi.var;

export namespace ksi {
	
	using namespace just::text_literals;

	struct space;
	struct function_body_base;

	using function_body_base_pointer = function_body_base *;

	struct call_space {
		using t_args = std::vector<var::any_var>;

		// data
		t_args	m_args;
		t_args	m_vars;

		call_space(function_body_base_pointer v_body);
	};

	struct stack {
		using t_items = just::array_alias<var::any_var, just::capacity_step<16, 16> >;

		// data
		t_items				m_items;
		var::var_pointer	m_var;

		var::any_var * last() { return m_items.end() -1; }

		void var_set(var::var_pointer p_var) {
			m_var = p_var;
		}

		void var_put() {
			just::array_append(m_items, *m_var);
		}

		void var_put_link() {
			just::array_append(m_items);
			last()->link_to(m_var);
		}

		void var_put_ref() {
			just::array_append(m_items);
			last()->ref_to(m_var);
		}
	};

	struct module_base {
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

		virtual t_text_value name() const = 0;

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

	//

	struct instr_data {
		using const_reference = const instr_data &;

		// data
		position	m_pos;
		t_integer	m_arg = 0;
		t_index		m_param = 0;
		t_index		m_group = 0;
	};

	struct instr_type {
		static void do_nothing(space * p_space, call_space * p_call, stack * p_stack,
			log_base::pointer p_log, instr_data::const_reference p_data
		) {}

		using t_fn = decltype(&do_nothing);
		using t_text = const t_text_value::t_impl_base *;
		using const_pointer = const instr_type *;

		// data
		t_text	m_name;
		t_fn	m_fn;

		bool empty() const { return m_fn == &do_nothing; }
	};

	struct instr {
		// data
		instr_type::const_pointer	m_type;
		instr_data					m_data;
	};

	struct instr_group {
		using t_instructions = std::vector<instr>;

		// data
		t_instructions	m_instructions;
	};

	//

	struct function_body_base {
		using t_args = just::hive<t_text_value, std::monostate, just::text_less>;

		// data
		module_base::pointer	m_module;
		t_args					m_args;
		t_args					m_vars;

		function_body_base() {
			m_vars.maybe_emplace("ret"_jt);
		}
	};

	struct function_body :
		public function_body_base,
		public just::node_list<function_body>,
		public just::bases::with_deleter<function_body *>
	{
		using pointer = function_body *;
		using t_groups = std::vector<instr_group>;

		// data
		log_pos		m_log_pos;
		t_groups	m_groups;
	};

	struct function {
		using t_over_type = std::map<var::type_pointer, function_body::pointer, std::ranges::less>;
		using t_over_category = std::map<var::category::pointer, function_body::pointer, std::ranges::less>;

		// data
		function_body::pointer	m_common = nullptr;
		t_over_type				m_by_type;
		t_over_category			m_by_category;
	};

	//

	call_space::call_space(function_body_base_pointer v_body) :
		m_args{v_body->m_args.count()},
		m_vars{v_body->m_vars.count()}
	{}

} // ns