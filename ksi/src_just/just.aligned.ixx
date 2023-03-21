module;

#include "../src/pre.h"

export module just.aligned;

export import just.std;
import <algorithm>;

export namespace just {

	template <t_size C_size, t_size C_align>
	struct with_aligned_data {
		static constexpr t_size
		s_size = C_size,
		s_align = C_align;

		// data
		alignas(s_align)
		t_byte
			m_aligned[s_size];

		constexpr with_aligned_data & base_aligned() { return *this; }
	};

	template <typename ... V_types>
	using with_aligned_data_as = with_aligned_data<
		std::max({sizeof(V_types) ...}),
		std::max({alignof(V_types) ...})
	>;

} // ns