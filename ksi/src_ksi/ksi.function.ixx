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
	struct function_body;

	using function_body_pointer = function_body *;

	struct call_space {
		using pointer = call_space *;
		using t_args = std::vector<var::any_var>;

		// data
		function_body_pointer	m_body;
		t_args					m_args;
		t_args					m_vars;

		call_space(function_body_pointer p_body);
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

	struct function_body {
		using pointer = function_body *;
		using t_args = just::hive<t_text_value, std::monostate, just::text_less>;

		// data
		module_pointer	m_module;
		t_args			m_args;
		t_args			m_vars;

		function_body(module_pointer p_module) : m_module{p_module} {
			m_vars.maybe_emplace("ret"_jt);
		}
	};

	struct function_body_user :
		public function_body,
		public just::node_list<function_body_user>,
		public just::bases::with_deleter<function_body_user *>
	{
		using pointer = function_body_user *;
		using t_groups = std::vector<instr_group>;

		// data
		log_pos		m_log_pos;
		t_groups	m_groups;

		function_body_user(module_pointer p_module, const log_pos & p_log_pos) :
			function_body{p_module}, m_log_pos{p_log_pos}
		{}
	};

	struct function :
		public var::with_name,
		public just::node_list<function>,
		public just::bases::with_deleter<function *>
	{
		using pointer = function *;
		using t_over_type = std::map<var::type_pointer, function_body::pointer, std::ranges::less>;
		using t_over_category = std::map<var::category::pointer, function_body::pointer, std::ranges::less>;

		// data
		function_body::pointer		m_common = nullptr;
		t_over_type					m_by_type;
		t_over_category				m_by_category;

		function(module_pointer	p_module, const t_text_value & p_name) : with_name{p_module} { name(p_name); }
	};

	//

	call_space::call_space(function_body_pointer p_body) :
		m_body{p_body},
		m_args{p_body->m_args.count()},
		m_vars{p_body->m_vars.count()}
	{}

} // ns