module;

#include "../src/pre.h"

export module ksi.function;

export import <vector>;
export import just.text;
export import just.array;
export import just.hive;
export import ksi.var;

export namespace ksi {
	
	struct space;
	struct context {};
	struct call_stack {};
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
			last()->link_to<true>(m_var);
		}
	};

	struct module_base {
		using t_types = just::hive<just::text, var::type_pointer, just::text_less>;
		using t_structs = just::list<var::type_struct, just::closers::compound_call_deleter<false>::template t_closer>;

		// data
		var::t_text_value	m_name;
		t_structs			m_structs;
		t_types				m_types, m_types_used;
	};

	struct instr_data {
		using const_reference = const instr_data &;

		// data
		position		m_pos;
		just::t_int		m_arg = 0, m_param = 0, m_group = 0;
	};

	struct instr_type {
		static void do_nothing(space * p_space, call_stack * p_call, stack * p_stack,
			log_base::pointer p_log, instr_data::const_reference p_data
		) {}

		using t_fn = decltype(&do_nothing);
		using t_text = const just::text::t_impl_base *;
		using const_pointer = const instr_type *;

		// data
		t_text	m_name;
		t_fn	m_fn;
		bool	m_nothing = false;
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

	struct function_body {
		using t_groups = std::vector<instr_group>;

		// data
		fs::path	m_path;
		t_groups	m_groups;
	};

} // ns