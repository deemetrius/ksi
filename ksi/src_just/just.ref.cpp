module;

export module just.ref;
export import just.common;
import <string_view>;
import <utility>;

export namespace just {

namespace closers {

template <typename T>
struct simple_none {
	using const_pointer = const T *;

	static void close(const_pointer) {}
};

template <typename T>
struct simple_destructor {
	using type = T;
	using pointer = type *;

	static void close(pointer h) { h->~type(); }
};

template <typename T>
struct simple_one {
	using const_pointer = const T *;

	static void close(const_pointer h) { delete h; }
};

template <typename T>
struct simple_many {
	using const_pointer = const T *;

	static void close(const_pointer h) { delete [] h; }
};

template <typename T>
struct simple_call_deleter {
	using const_pointer = const T *;

	static void close(const_pointer h) { h->deleter_(h); }
};

template <bool Check_null = true, template <typename T1> typename Closer = simple_one>
struct compound_cnt {
	template <typename T>
	struct closer {
		using t_closer = Closer<T>;
		using pointer = T *;

		static void close(pointer h) {
			if constexpr ( Check_null ) { if( h && h->refs_dec() ) t_closer::close(h); }
			else { if( h->refs_dec() ) t_closer::close(h); }
		}
	};
};

template <bool Check_null = true>
struct compound_cnt_call_deleter {
	template <typename T>
	struct closer {
		using pointer = T *;
		using const_pointer = const T *;
		using tfn_deleter = decltype( std::declval<pointer>()->refs_dec() );

		static void close(pointer h) {
			if constexpr ( Check_null ) {
				if( h ) { if( auto deleter = h->refs_dec() ) deleter(h); }
			} else { if( auto deleter = h->refs_dec() ) deleter(h); }
		}
	};
};

} // ns closers

namespace bases {

template <typename T>
struct with_handle {
	T * h_ = nullptr;

	friend auto operator <=> (const with_handle &, const with_handle &) = default;
};

template <typename T>
struct with_handle_mutable {
	mutable T * h_ = nullptr;

	friend auto operator <=> (const with_handle_mutable &, const with_handle_mutable &) = default;
};

struct with_ref_count {
	id refs_ = 1;

	void refs_inc() { ++refs_; }
	bool refs_dec() { --refs_; return refs_ < 1; }
};

struct with_ref_count_mutable {
	mutable id refs_ = 1;

	void refs_inc() const { ++refs_; }
	bool refs_dec() const { --refs_; return refs_ < 1; }
};

template <typename T, template <typename T1> typename Closer = closers::simple_one>
struct with_deleter {
	using t_closer = Closer<T>;
	using tfn_deleter = decltype(&t_closer::close);

	tfn_deleter deleter_ = t_closer::close;
};

} // ns bases

template <template <typename T1> typename Closer = closers::simple_one>
struct traits_ref_unique {
	template <typename T>
	struct inner :
		public Closer<T>
	{
		using ref_base = bases::with_handle_mutable<T>;
		using pointer = T *;

		static void accept_init(pointer & to, pointer & from) { std::swap(to, from); }

		static void accept(pointer & to, pointer & from) { std::swap(to, from); }
	};
};

template <bool Check_null, template <typename T1> typename Closer>
struct traits_ref_cnt {
	template <typename T>
	struct inner :
		public Closer<T>
	{
		using ref_base = bases::with_handle<T>;
		using t_closer = Closer<T>;
		using pointer = T *;

		static void accept_init(pointer & to, pointer from) {
			if constexpr ( Check_null ) { if( from ) from->refs_inc(); }
			else { from->refs_inc(); }
			to = from;
		}

		static void accept(pointer & to, pointer from) {
			if constexpr ( Check_null ) { if( from ) from->refs_inc(); }
			else { from->refs_inc(); }
			t_closer::close(to);
			to = from;
		}
	};
};

//

template <typename T, typename Traits>
struct ref :
	public Traits::inner<T>::ref_base
{
	using traits = typename Traits::inner<T>;
	using base = traits::ref_base;
	using pointer = traits::pointer;

	friend void swap(ref & r1, ref & r2) { std::swap(r1.h_, r2.h_); }

	ref(pointer h) : base{h} {}
	ref & operator = (pointer h) {
		traits::close(this->h_);
		this->h_ = h;
		return *this;
	}

	//
	ref() = default;
	~ref() { traits::close(this->h_); }

	// copy
	ref(const ref & r) { traits::accept_init(this->h_, r.h_); }
	ref & operator = (const ref & r) {
		traits::accept(this->h_, r.h_);
		return *this;
	}

	//
	operator bool () const { return this->h_; }
	pointer operator -> () const { return this->h_; }
	T & operator * () const { return *this->h_; }
};

} // ns