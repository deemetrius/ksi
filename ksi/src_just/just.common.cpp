module;

export module just.common;
import <type_traits>;
import <concepts>;
import <cstddef>;
import <utility>;

export namespace just {

using id = std::ptrdiff_t;
using uid = std::size_t;

struct case_default;
struct case_none;
struct case_exact;
struct case_cross;

//

template <typename Enum>
constexpr std::underlying_type_t<Enum> to_underlying(Enum val) noexcept {
	return static_cast< std::underlying_type_t<Enum> >(val);
}

using byte_underlying = unsigned char;
enum class byte : byte_underlying { zero };

//

template <typename T>
concept c_scalar = std::is_scalar_v<T>;

template <typename T, typename ... U>
concept c_any_of = ( std::same_as<T, U> || ... );

template <typename T, typename Left>
concept c_inequal_comparable_with_left = requires(const T & target, const Left & left) {
	static_cast<bool>(left != target);
};

template <typename T, bool Can_change = false>
using arg_passing_t = std::conditional_t<std::is_scalar_v<T>, T,
	std::conditional_t<Can_change, T &, const T &>
>;

//

/*template <c_scalar T>
constexpr T max(T a) { return a; }*/

template <c_scalar T>
constexpr T max(T a, std::type_identity_t<T> b) { return (a < b) ? b : a; }

template <c_scalar T, c_scalar ... Rest>
constexpr T max(T a, std::type_identity_t<T> b, Rest ... rest) { return max( a, max(b, rest ...) ); }

//

/*template <typename From, typename To>
constexpr void copy_n(From from, uid n, To to) {
	for( uid i{}; i < n; ++i, ++from, ++to ) *to = *from;
}*/

template <typename C, uid N>
struct fixed_string {
	using type = C;
	enum n_size : uid { len = N -1 };

	// data
	type s_[N];

	constexpr fixed_string() requires(N == 1) : s_{0} {}

	template <uid ... I>
	constexpr fixed_string(const type (&s)[N], std::index_sequence<I ...>) : s_{s[I] ...} {}

	constexpr fixed_string(const type (&s)[N]) : fixed_string( s, std::make_index_sequence<N>() ) {}
};

template <uid N, uid Align>
struct alignas(Align) aligned_data {
	using type = byte;

	// data
	type data_[N];
};

//

template <typename Iter_begin, typename Iter_end = Iter_begin>
struct range {
	Iter_begin begin_;
	Iter_end end_;

	constexpr Iter_begin begin() const { return begin_; }
	constexpr Iter_end end() const { return end_; }
};

template <typename Iter>
struct reverse_iterator {
	using t_iter = Iter;

	// data
	t_iter it_;

	constexpr reverse_iterator(t_iter it) : it_(it) {}
	constexpr auto & operator * () const { return *it_; }
	constexpr reverse_iterator & operator ++ () { --it_; return *this; }

	constexpr bool operator != (const reverse_iterator & it2) const {
		return it_ != it2.it_;
	}
	template <c_inequal_comparable_with_left<t_iter> Iterator2>
	constexpr bool operator != (const Iterator2 & it2) const {
		return it_ != it2;
	}

	/*template <std::three_way_comparable_with<t_iter> Iterator2>
	constexpr std::compare_three_way_result_t<t_iter, Iterator2>
	operator <=> (const reverse_iterator<Iterator2> & it2) const {
		return it_ <=> it2.it_;
	}
	template <typename Iterator2>
	constexpr bool operator == (const reverse_iterator<Iterator2> & it2) const {
		return it_ == it2.it_;
	}*/
};

} // ns