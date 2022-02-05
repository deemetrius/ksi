module;

export module just.aux;
export import just.common;
import <type_traits>;

export namespace just {

namespace closers {

template <typename T>
struct simple_none {
	using const_pointer = const T *;

	static constexpr bool can_accept_null = true;

	static void close(const_pointer) {}
};

template <typename T>
struct simple_destructor {
	using type = T;
	using const_pointer = const type *;

	static constexpr bool can_accept_null = false;

	static void close(const_pointer h) { h->~type(); }
};

template <typename T>
struct simple_one {
	using const_pointer = const T *;

	static constexpr bool can_accept_null = true;

	static void close(const_pointer h) { delete h; }
};

template <typename T>
struct simple_many {
	using const_pointer = const T *;

	static constexpr bool can_accept_null = true;

	static void close(const_pointer h) { delete [] h; }
};

template <typename T>
struct simple_call_deleter {
	using const_pointer = const T *;

	static constexpr bool can_accept_null = false;

	static void close(const_pointer h) { h->deleter_(h); }
};

template <typename T>
struct simple_call_deleter_maybe {
	using const_pointer = const T *;

	static constexpr bool can_accept_null = true;

	static void close(const_pointer h) { if( h ) h->deleter_(h); }
};

template <typename Cast, bool Const_close = false, template <typename T1> typename Closer = simple_one>
struct compound_cast {
	using target = Cast;
	using target_pointer = target *;
	using target_const_pointer = const target *;
	using target_pass = std::conditional_t<Const_close, target_const_pointer, target_pointer>;
	using t_closer = Closer<target>;

	template <typename T>
	struct closer {
		using type = T;
		using pointer = type *;
		using const_pointer = const type *;
		using t_pass = std::conditional_t<Const_close, const_pointer, pointer>;

		static constexpr bool can_accept_null = t_closer::can_accept_null;

		static void close(t_pass h) { t_closer::close( static_cast<target_pass>(h) ); }
	};
};

template <bool Check_null = true, bool Const_close = false, template <typename T1> typename Closer = simple_one>
struct compound_cnt {
	template <typename T>
	struct closer {
		using type = T;
		using pointer = type *;
		using const_pointer = const type *;
		using t_closer = Closer<type>;
		using t_pass = std::conditional_t<Const_close, const_pointer, pointer>;

		static constexpr bool can_accept_null = Check_null;

		static void close(t_pass h) {
			if constexpr ( Check_null ) { if( h && h->refs_dec() ) t_closer::close(h); }
			else { if( h->refs_dec() ) t_closer::close(h); }
		}
	};
};

template <bool Check_null = true>
struct compound_cnt_call_deleter {
	template <typename T>
	struct closer {
		using type = T;
		using pointer = type *;
		using const_pointer = const type *;
		//using tfn_deleter = decltype( std::declval<const_pointer>()->refs_dec() );

		static constexpr bool can_accept_null = Check_null;

		static void close(const_pointer h) {
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
	bool refs_dec() { return --refs_ < 1; }
};

struct with_ref_count_mutable {
	mutable id refs_ = 1;

	void refs_inc() const { ++refs_; }
	bool refs_dec() const { return --refs_ < 1; }
};

template <typename T, template <typename T1> typename Closer = closers::simple_one>
struct with_deleter {
	using t_closer = Closer<T>;
	using tfn_deleter = decltype(&t_closer::close);

	tfn_deleter deleter_ = t_closer::close;
};

} // ns bases

} // ns just