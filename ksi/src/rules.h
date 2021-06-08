#pragma once
#include "tokens.h"

namespace ksi {
namespace rules {

struct info {
	wtext path_;
	bool good_ = false;
};

struct rule_start {
	static void parse(const Char * str, info & inf, t_tokens & toks, base_log * log);
};

} // ns
} // ns
