module;

export module just.ref;
export import just.aux;
import <concepts>;

export namespace just {

// ref trait unique

template <template <typename T1> typename Closer = closers::simple_one>
struct traits_ref_unique {
	template <typename T>
	struct inner :
		public Closer<T>
	{
		using ref_base = bases::with_handle_mutable<T>;
		using pointer = T *;

		static constexpr bool allow_default = true;

		static void accept_init(pointer & to, pointer & from) { std::ranges::swap(to, from); }

		static void accept(pointer & to, pointer & from) { std::ranges::swap(to, from); }
	};
};

// ref trait cnt

template <bool Check_null, template <typename T1> typename Closer>
struct traits_ref_cnt {
	template <typename T>
	struct inner :
		public Closer<T>
	{
		using ref_base = bases::with_handle<T>;
		using t_closer = Closer<T>;
		using pointer = T *;

		static constexpr bool allow_default = Check_null;

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

} // ns just