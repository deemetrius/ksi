module;

export module just.common;
import <compare>;
import <type_traits>;
import <concepts>;
import <cstddef>;

export namespace just {

using id = std::ptrdiff_t;
using uid = std::size_t;

//

template <typename Enum>
constexpr inline std::underlying_type_t<Enum> to_underlying(Enum val) noexcept {
	return static_cast< std::underlying_type_t<Enum> >(val);
}

//

template <typename T>
concept c_scalar = std::is_scalar_v<T>;

template <typename T, typename ... U>
concept c_any_of = ( std::same_as<T, U> || ... );

template <typename T, bool Can_change = false>
using arg_passing_t = std::conditional_t<std::is_scalar_v<T>, T,
	std::conditional_t<Can_change, T &, const T &>
>;

//

template <c_scalar T>
constexpr T max(T a) { return a; }

template <c_scalar T>
constexpr T max(T a, T b) { return (a < b) ? b : a; }

template <c_scalar T, c_scalar ... Rest>
constexpr T max(T a, Rest ... b) { return max( a, max(b ...) ); }

//

template <typename From, typename To>
constexpr void copy_n(From from, uid n, To to) {
	for( uid i{}; i < n; ++i, ++from, ++to ) *to = *from;
}

template <typename C, uid N>
struct fixed_string {
	using type = C;
	enum n_size : uid { len = N -1 };

	// data
	type s_[N];

	constexpr fixed_string() requires(N == 1) { *s_ = 0; }
	constexpr fixed_string(const type (&s)[N]) {
		copy_n(s, N, s_);
	}
};

template <uid N, uid Align>
struct alignas(Align) aligned_data {
	using type = unsigned char;

	// data
	type data_[N];
};

//

template <typename IterBegin, typename IterEnd = IterBegin>
struct range {
	IterBegin begin_;
	IterEnd end_;

	constexpr IterBegin begin() const { return begin_; }
	constexpr IterEnd end() const { return end_; }
};

template <typename Iter>
struct reverse_iterator {
	using t_iter = Iter;

	t_iter it_;

	constexpr reverse_iterator(t_iter it) : it_(it) {}
	constexpr auto & operator * () const { return *it_; }
	constexpr reverse_iterator & operator ++ () { --it_; return *this; }

	template <std::three_way_comparable_with<t_iter> Iterator2>
	constexpr std::compare_three_way_result_t<t_iter, Iterator2>
	operator <=> (const reverse_iterator<Iterator2> & it2) const {
		return it_ <=> it2.it_;
	}
	template <typename Iterator2>
	constexpr bool operator == (const reverse_iterator<Iterator2> & it2) const {
		return it_ == it2.it_;
	}
};

} // ns