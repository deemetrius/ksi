module;

#include "../src/pre.h"

export module ksi.instructions;

export import ksi.function;

export namespace ksi {

	using namespace just::text_literals;
	//using namespace std::literals::string_view_literals;

	struct instructions {

		static void do_pop(space * p_space, call_space * p_call, stack * p_stack,
			log_base::pointer p_log, instr_data::const_reference p_data
		) {
			p_stack->var_remove(1);
		}

		static void do_put_null(space * p_space, call_space * p_call, stack * p_stack,
			log_base::pointer p_log, instr_data::const_reference p_data
		) {
			p_stack->var_add(var::variant_null{});
		}

		static void do_put_all(space * p_space, call_space * p_call, stack * p_stack,
			log_base::pointer p_log, instr_data::const_reference p_data
		) {
			p_stack->var_add(var::variant_all{});
		}

		static void do_put_bool(space * p_space, call_space * p_call, stack * p_stack,
			log_base::pointer p_log, instr_data::const_reference p_data
		) {
			p_stack->var_add(static_cast<bool>(p_data.m_arg) );
		}

		static void do_put_int(space * p_space, call_space * p_call, stack * p_stack,
			log_base::pointer p_log, instr_data::const_reference p_data
		) {
			p_stack->var_add(p_data.m_arg);
		}

		static const instr_type
		s_pop,
		s_put_null,
		s_put_all,
		s_put_bool,
		s_put_int;

	}; // struct

	const instr_type
	instructions::s_pop			{"do_pop"_jt,			&do_pop},
	instructions::s_put_null	{"do_put_null"_jt,		&do_put_null},
	instructions::s_put_all		{"do_put_all"_jt,		&do_put_all},
	instructions::s_put_bool	{"do_put_bool"_jt,		&do_put_bool},
	instructions::s_put_int		{"do_put_int"_jt,		&do_put_int};

} // ns