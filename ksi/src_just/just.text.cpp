module;

export module just.text;
export import just.ref;
import <concepts>;
import <string_view>;
import <iostream>;

export namespace just {

namespace detail {

template <typename C>
struct impl_text_base {
	using const_pointer = const C *;
	using tfn_deleter = bases::with_deleter< impl_text_base<C> >::tfn_deleter;

	const_pointer cs_;
	id len_;

	constexpr virtual ~impl_text_base() = default;
	constexpr virtual void refs_inc() const {}
	constexpr virtual tfn_deleter refs_dec() const { return nullptr; }

	constexpr impl_text_base(const_pointer cs, id len) : cs_(cs), len_(len) {}
};

template <typename C>
struct impl_text :
	public impl_text_base<C>,
	public bases::with_deleter< impl_text_base<C> >
{
	using base = impl_text_base<C>;
	using const_pointer = const C *;
	using pointer = C *;
	using tfn_deleter = base::tfn_deleter;

	mutable id refs_ = 1;
	pointer s_;

	void refs_inc() const override { ++refs_; }
	tfn_deleter refs_dec() const override { --refs_; return refs_ < 1 ? this->deleter_ : nullptr; }

	impl_text(pointer s, id len) : base(s, len), s_(s) {}
	~impl_text() { delete [] s_; }
};

} // ns

template <typename C>
struct basic_text {
	using type = C;
	using pointer = type *;
	using const_pointer = const type *;
	//
	using t_impl_base = detail::impl_text_base<type>;
	using t_impl = detail::impl_text<type>;
	using t_ref = ref<const t_impl_base,
		traits_ref_cnt<false, closers::compound_cnt_call_deleter<false>::closer>
	>;

	friend void swap(basic_text & t1, basic_text & t2) { std::ranges::swap(t1.ref_, t2.ref_); }

	t_ref ref_;

	basic_text(const t_impl_base * impl) : ref_(impl) {}
	basic_text & operator = (const t_impl_base * impl) {
		ref_ = impl;
		return *this;
	}

	explicit basic_text(id len) {
		pointer s;
		ref_ = new t_impl{s = new type[len +1], len};
		*s = 0;
	}

	//
	operator bool () const { return *ref_->cs_; }
	const t_impl_base * operator -> () const { return ref_.h_; }
	operator std::basic_string_view<C> () const { return {ref_->cs_, ref_->len_}; }
};

template <typename C, typename T>
std::basic_ostream<C> & operator << (std::basic_ostream<C, T> & os, const basic_text<C> & tx) {
	return os << tx->cs_;
}

using text = basic_text<char>;
using wtext = basic_text<wchar_t>;

namespace detail {

template <fixed_string V>
struct static_data {
	using type = std::remove_cvref_t<decltype(V)>;
	using t_impl_base = basic_text<typename type::type>::t_impl_base;
	static constexpr const t_impl_base value{V.s_, type::len};
};

} // ns

namespace text_literals {

template <fixed_string V>
constexpr auto operator "" _jt () {
	return &detail::static_data<V>::value;
}

} // ns
} // ns