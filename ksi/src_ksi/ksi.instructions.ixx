module;

#include "../src/pre.h"

export module ksi.instructions;

export import ksi.function;
import ksi.space;
import ksi.var.ops;

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

		// param - literal id
		static void do_put_literal(space * p_space, call_space * p_call, stack * p_stack,
			log_base::pointer p_log, instr_data::const_reference p_data
		) {
			p_stack->var_add((*p_space->m_literals.m_keys[p_data.m_param]).first);
		}

		// param - var id
		static void do_put_var(space * p_space, call_space * p_call, stack * p_stack,
			log_base::pointer p_log, instr_data::const_reference p_data
		) {
			p_stack->var_add(p_call->m_vars[p_data.m_param]);
		}

		// param - var id
		static void do_put_var_link(space * p_space, call_space * p_call, stack * p_stack,
			log_base::pointer p_log, instr_data::const_reference p_data
		) {
			p_stack->var_link(&p_call->m_vars[p_data.m_param]);
		}

		// param - var id
		static void do_put_var_ref(space * p_space, call_space * p_call, stack * p_stack,
			log_base::pointer p_log, instr_data::const_reference p_data
		) {
			p_stack->var_ref(&p_call->m_vars[p_data.m_param]);
		}

		template <typename T_op>
		static void do_math(space * p_space, call_space * p_call, stack * p_stack,
			log_base::pointer p_log, instr_data::const_reference p_data
		) {
			p_stack->last(1) = var::math<T_op>(p_stack->last(1), p_stack->last() );
			p_stack->var_remove(1);
		}

		template <typename T_op>
		static void do_math_bits(space * p_space, call_space * p_call, stack * p_stack,
			log_base::pointer p_log, instr_data::const_reference p_data
		) {
			p_stack->last(1) = var::math_bits<T_op>(p_stack->last(1), p_stack->last() );
			p_stack->var_remove(1);
		}

		static const instr_type
		s_pop,
		s_put_null,
		s_put_all,
		s_put_bool,
		s_put_int,
		s_put_literal,
		s_put_var,
		s_put_var_link,
		s_put_var_ref,
		s_op_add,
		s_op_subtract,
		s_op_multiply,
		s_op_divide,
		s_op_bits_and,
		s_op_bits_xor,
		s_op_bits_or,
		s_op_bits_shift,
		s_op_modulo;

	}; // struct

	const instr_type
	instructions::s_pop				{"do_pop"_jt,			&do_pop},
	instructions::s_put_null		{"do_put_null"_jt,		&do_put_null},
	instructions::s_put_all			{"do_put_all"_jt,		&do_put_all},
	instructions::s_put_bool		{"do_put_bool"_jt,		&do_put_bool},
	instructions::s_put_int			{"do_put_int"_jt,		&do_put_int},
	instructions::s_put_literal		{"do_put_literal"_jt,	&do_put_literal},
	instructions::s_put_var			{"do_put_var"_jt,		&do_put_var},
	instructions::s_put_var_link	{"do_put_var_link"_jt,	&do_put_var_link},
	instructions::s_put_var_ref		{"do_put_var_ref"_jt,	&do_put_var_ref},
	instructions::s_op_add			{"do_op_add"_jt,		&do_math<var::op_add>},
	instructions::s_op_subtract		{"do_op_subtract"_jt,	&do_math<var::op_subtract>},
	instructions::s_op_multiply		{"do_op_multiply"_jt,	&do_math<var::op_multiply>},
	instructions::s_op_divide		{"do_op_divide"_jt,		&do_math<var::op_divide>},
	instructions::s_op_modulo		{"do_op_modulo"_jt,		&do_math<var::op_modulo>},
	instructions::s_op_bits_shift	{"do_op_bits_shift"_jt,	&do_math_bits<var::op_bits_shift>},
	instructions::s_op_bits_and		{"do_op_bits_and"_jt,	&do_math_bits<var::op_bits_and>},
	instructions::s_op_bits_xor		{"do_op_bits_xor"_jt,	&do_math_bits<var::op_bits_xor>},
	instructions::s_op_bits_or		{"do_op_bits_or"_jt,	&do_math_bits<var::op_bits_or>};

} // ns