module;

#include "../src/pre.h"

export module ksi.parser:rules;

export import :state;
export import :basic;
export import :alt;

namespace ksi {

	namespace parser {

		using namespace std::string_literals;

		struct nest {

			#include "ksi.parser.module.h"

			struct rule_file : public rule_alt<false,
				t_eof,
				t_opt_space,
				t_module_def
			> {};

			struct rule_module : public rule_alt<false,
				t_eof,
				t_opt_space,
				t_module_def,
				t_var_mod
			> {};

			struct rule_module_after_var : public rule_alt<false,
				t_opt_space,
				t_var_mod_assign
			> {};

			struct rule_module_after_assign : public rule_alt_back<rule_module,
				t_opt_space,
				t_literal_int
			> {};

		}; // nest

	} // ns

} // ns