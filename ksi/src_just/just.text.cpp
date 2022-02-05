module;

export module just.text;
export import just.ref;
import <concepts>;
import <string_view>;
import <iostream>;

export namespace just {

namespace detail {

template <typename C>
struct impl_text;

template <typename C>
struct impl_text_base {
	using type = C;
	using pointer = type *;
	using const_pointer = const type *;
	using t_with = bases::with_deleter<
		const impl_text_base<C>,
		closers::compound_cast<const impl_text<C>, true>::template closer
	>;
	using tfn_deleter = t_with::tfn_deleter;

	// data
	const_pointer cs_;
	id len_;

	virtual void refs_inc() const {}
	virtual tfn_deleter refs_dec() const { return nullptr; }

	constexpr impl_text_base(const_pointer cs, id len) : cs_(cs), len_(len) {}
};

template <typename C>
struct impl_text :
	public impl_text_base<C>,
	public impl_text_base<C>::t_with
{
	using type = C;
	using base = impl_text_base<type>;
	using pointer = base::pointer;
	using const_pointer = base::const_pointer;
	using tfn_deleter = base::tfn_deleter;

	// data
	mutable id refs_ = 1;

	void refs_inc() const override { ++refs_; }
	tfn_deleter refs_dec() const override { return --refs_ < 1 ? this->deleter_ : nullptr; }

	impl_text(pointer & s, id len) : base(s = new type[len +1], len) {}
	~impl_text() { delete [] this->cs_; }
};

template <typename T> T plain(const T &);

template <fixed_string V>
struct static_data {
	using type = decltype( plain(*V.s_) );
	using t_impl_base = impl_text_base<type>;
	
	static constexpr const t_impl_base value{V.s_, V.len};
};

} // ns detail

namespace text_literals {

template <fixed_string V>
constexpr auto operator "" _jt () {
	return &detail::static_data<V>::value;
}

} // ns text_literals

template <typename C>
struct basic_text {
	using type = C;
	using t_impl_base = detail::impl_text_base<type>;
	using t_impl = detail::impl_text<type>;
	using t_ref = ref<const t_impl_base,
		traits_ref_cnt<false, closers::compound_cnt_call_deleter<false>::closer>
	>;
	using pointer = t_impl_base::pointer;
	using const_pointer = t_impl_base::const_pointer;

	static constexpr const type empty[1]{};

	friend void swap(basic_text & t1, basic_text & t2) { std::ranges::swap(t1.ref_, t2.ref_); }

	// data
	t_ref ref_;

	basic_text() : basic_text(&detail::static_data<empty>::value) {}
	basic_text(const t_impl_base * impl) : ref_(impl) {}
	basic_text & operator = (const t_impl_base * impl) {
		ref_ = impl;
		return *this;
	}

	basic_text(id len, pointer & s) : ref_( new t_impl(s, len) ) { *s = 0; }

	//
	operator bool () const { return *ref_->cs_; }
	bool operator ! () const { return !*ref_->cs_; }
	const t_impl_base * operator -> () const { return ref_.h_; }
	operator std::basic_string_view<type> () const { return {ref_->cs_, ref_->len_}; }
};

template <typename C, typename T>
std::basic_ostream<C, T> & operator << (std::basic_ostream<C, T> & os, const basic_text<C> & tx) {
	return os << tx->cs_;
}

using text = basic_text<char>;
using wtext = basic_text<wchar_t>;

} // ns just