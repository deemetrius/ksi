module;
#include <cstddef>

export module just.common;
import <algorithm>;

export namespace just {

using id = std::ptrdiff_t;
using uid = std::size_t;

template <typename C, uid N>
struct fixed_string {
	using type = C;
	enum n_size : uid { len = N -1 };
	type s_[N];

	constexpr fixed_string(const type (&s)[N]) {
		std::copy_n(s, N, s_);
	}
};

} // ns