module;

export module just.compare;
export import just.common;
import <compare>;

export namespace just {

template <typename Result, std::unsigned_integral T>
constexpr Result sign(T val) {
	return val > 0;
}

template <typename Result, std::signed_integral T>
constexpr Result sign(T val) {
	return (val > 0) - (val < 0);
}

template <typename Result, std::floating_point T>
constexpr Result sign(T val, Result on_nan = {}) {
	return (val != val) ? on_nan : (val > 0.0) - (val < 0.0);
}

template <typename Result, c_any_of<std::strong_ordering, std::weak_ordering> T>
constexpr Result sign(T val) {
	return (val > 0) - (val < 0);
}

template <typename Result>
constexpr Result sign(std::partial_ordering val, Result on_unordered) {
	return (val == std::partial_ordering::unordered) ? on_unordered : ((val > 0) - (val < 0));
}

//

using compare_result = int;
enum n_compare : compare_result { cmp_less = -1, cmp_equal = 0, cmp_greater = +1 };

template <typename Ordering = compare_result>
struct compare_helper;

template <>
struct compare_helper<compare_result> {
	using t_ordering = compare_result;
	enum : t_ordering { less = cmp_less, equal = cmp_equal, greater = cmp_greater };
};

namespace detail {

template <typename Ordering>
struct compare_helper {
	using t_ordering = Ordering;

	static constexpr t_ordering
	less	= t_ordering::less,
	equal	= t_ordering::equivalent,
	greater	= t_ordering::greater;
};

} // ns detail

template <>
struct compare_helper<std::strong_ordering> : public detail::compare_helper<std::strong_ordering> {};
template <>
struct compare_helper<std::weak_ordering> : public detail::compare_helper<std::weak_ordering> {};
template <>
struct compare_helper<std::partial_ordering> : public detail::compare_helper<std::partial_ordering> {
	using base = detail::compare_helper<std::partial_ordering>;

	static constexpr base::t_ordering
	unordered = base::t_ordering::unordered;
};

//

/*
template <typename Ordering>
requires requires { Ordering::less; Ordering::equivalent; Ordering::greater; }
struct sign_to_ordering {
	static constexpr const Ordering
	values[]{Ordering::less, Ordering::equivalent, Ordering::greater},
	* mid = values + 1;
};

template <>
struct sign_to_ordering<std::partial_ordering> {
	using t_ordering = std::partial_ordering;

	static constexpr const t_ordering
	values[]{t_ordering::less, t_ordering::equivalent, t_ordering::greater, t_ordering::unordered},
	* mid = values + 1;
};

//

template <typename T, T ... Elements>
requires ( sizeof...(Elements) >= 3 )
struct sign_to_custom {
	static constexpr const T
	values[]{Elements ...},
	* mid = values + 1;
};

template <typename T, c_any_of<std::strong_ordering, std::weak_ordering, std::partial_ordering> Order>
using sign_to_custom_auto = std::conditional_t<std::is_same_v<Order, std::strong_ordering>,
	sign_to_custom<T, T::less, T::equal_strong, T::greater>, std::conditional_t<std::is_same_v<Order, std::weak_ordering>,
	sign_to_custom<T, T::less, T::equal_weak, T::greater>,
	sign_to_custom<T, T::less, T::equal_partial, T::greater, T::unordered>
> >;

//

template <typename Result, c_any_of<std::strong_ordering, std::weak_ordering> Order>
constexpr Result cast_ordering(Order from) {
	using t_helper = sign_to_custom_auto<Result, Order>;
	return t_helper::mid[sign<id>(from)];
}

template <typename Result>
constexpr Result cast_ordering(std::partial_ordering from) {
	using t_helper = sign_to_custom_auto<Result, std::partial_ordering>;
	return t_helper::mid[sign<id>(from, +2)];
}
*/

/*
enum class compare_strict {
	unordered = -2,

	less = -1,
	greater = +1,

	equal_strong = 0,
	equal_weak = 2,
	equal_partial = 3
};

enum class compare_simple {
	less = -1,
	equal_any = 0,
	greater = +1,

	equal_strong = equal_any,
	equal_weak = equal_any,
	equal_partial = equal_any,

	unordered = less
};
*/

} // ns just