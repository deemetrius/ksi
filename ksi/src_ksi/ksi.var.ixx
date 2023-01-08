module;

#include "../src/pre.h"

export module ksi.var2;

export import just.keep;
export import just.optr;

export namespace ksi {

	namespace var {
	
		using ring = just::owned_ring;
		using with_ring = just::with_ring<ring>;

		struct type {};

		struct value : with_ring::is_owned<value> {
			void unset_elements() {}
		};

	} // ns

} // ns