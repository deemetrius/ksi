module;
#include <cstddef>

export module just.common;
import <compare>;

export namespace just {

using id = std::ptrdiff_t;
using uid = std::size_t;

template <typename From, typename To>
constexpr void copy_n(From from, uid n, To to) {
	for( uid i{}; i < n; ++i, ++from, ++to ) *to = *from;
}

template <typename C, uid N>
struct fixed_string {
	using type = C;
	enum n_size : uid { len = N -1 };
	type s_[N];

	constexpr fixed_string(const type (&s)[N]) {
		copy_n(s, N, s_);
	}

	//constexpr uid size() const { return len; }
};

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
	constexpr auto operator * () const { return *it_; }
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