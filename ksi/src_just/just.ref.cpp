module;

export module just.ref;
export import just.aux;
import <concepts>;

export namespace just {

namespace detail {

struct init_null {
	static inline constexpr auto get_default() { return nullptr; }
};

} // ns detail

// ref trait unique

template <template <typename T1> typename Closer = closers::simple_one, typename Init = detail::init_null>
struct traits_ref_unique {
	template <typename T>
	struct inner :
		public Closer<T>
	{
		using ref_base = bases::with_handle_mutable<T>;
		using pointer = T *;
		using t_closer = Closer<T>;
		using t_init = Init;

		static constexpr bool need_init_call = !std::is_same_v<t_init, detail::init_null>;
		static constexpr bool allow_default = t_closer::can_accept_null;
		static constexpr bool allow_copy = t_closer::can_accept_null || need_init_call;

		static void accept_init(pointer & to, pointer & from) {
			if constexpr( need_init_call ) { to = t_init::get_default(); }
			std::ranges::swap(to, from);
		}

		static void accept(pointer & to, pointer & from) { std::ranges::swap(to, from); }
	};
};

// ref trait cnt

template <bool Check_null, template <typename T1> typename Closer>
struct traits_ref_cnt {
	template <typename T>
	requires ( Check_null == false || Closer<T>::can_accept_null )
	struct inner :
		public Closer<T>
	{
		using ref_base = bases::with_handle<T>;
		using pointer = T *;
		using t_closer = Closer<T>;

		static constexpr bool allow_default = Check_null;
		static constexpr bool allow_copy = true;

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

// ref

template <typename T, typename Traits>
struct ref :
	public Traits::inner<T>::ref_base
{
	using traits = typename Traits::inner<T>;
	using base = traits::ref_base;
	using pointer = traits::pointer;

	friend void swap(ref & r1, ref & r2) { std::ranges::swap(r1.h_, r2.h_); }

	ref(pointer h) : base{h} {}
	ref & operator = (pointer h) {
		traits::close(this->h_);
		this->h_ = h;
		return *this;
	}

	//
	ref() requires( traits::allow_default ) = default;
	~ref() { traits::close(this->h_); }

	// copy
	ref(const ref & r) requires( traits::allow_copy ) { traits::accept_init(this->h_, r.h_); }
	ref & operator = (const ref & r) {
		traits::accept(this->h_, r.h_);
		return *this;
	}

	//
	operator bool () const { return this->h_; }
	pointer operator -> () const { return this->h_; }
	T & operator * () const { return *this->h_; }
};

} // ns just