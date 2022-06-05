module;

#include "../src/pre.h"

export module ksi.function;

export import <vector>;
export import just.text;
export import ksi.log;

export namespace ksi {
	
	struct space;
	struct context {};
	struct call_stack {};
	struct stack {};

	struct instr_data {
		using const_reference = const instr_data &;

		// data
		position		m_pos;
		just::t_int		m_arg = 0, m_param = 0, m_group = 0;
	};

	struct instr_type {
		static void do_nothing(space * p_space, call_stack * p_call, stack * p_stack,
			log_base * p_log, instr_data::const_reference p_data
		) {}

		using t_fn = decltype(&do_nothing);
		using t_text = const just::text::t_impl_base *;
		using const_pointer = const instr_type *;

		// data
		t_text	m_name;
		t_fn	m_fn;
	};

	struct instr {
		// data
		instr_type::const_pointer	m_type;
		instr_data					m_data;
	};

	struct instr_group {
		using t_vector = std::vector<instr>;

		// data
		t_vector	m_instructions;
	};

	struct function_body {
		using t_vector = std::vector<instr_group>;

		// data
		t_vector	m_groups;
	};

} // ns