module;

#include "../src/pre.h"

export module ksi.instructions;

export import ksi.function;

export namespace ksi {

	using namespace just::text_literals;

	struct instructions {
		static constexpr instr_type
			s_do_nothing{"do_nothing"_jt, &instr_type::do_nothing, true}
		;
	};

} // ns