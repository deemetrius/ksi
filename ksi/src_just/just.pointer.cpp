module;

export module just.pointer;
export import just.ref;

export namespace just {

enum pointer_kind { pk_weak, pk_strong };

namespace detail {

template <typename T, template <typename T1> typename Closer>
struct impl_pointer {
	using type = T;
	using pointer = T *;
	using t_closer = Closer<T>;
	using t_deleter = bases::with_deleter< impl_pointer<T, Closer> >;
	using tfn_deleter = t_deleter::tfn_deleter;

	// data
	id refs_strong_, refs_weak_;
	pointer h_ = nullptr;
	tfn_deleter deleter_ = t_deleter::t_closer::close;

	// no copy
	impl_pointer(const impl_pointer &) = delete;
	impl_pointer & operator = (const impl_pointer &) = delete;

	inline void close_only() { t_closer::close(h_); }
	void close() { close_only(); h_ = nullptr; }
	inline void init_care(pointer obj) { h_ = obj; }
	void change(pointer obj) { close_only(); h_ = obj; }

	id refs_total() const { return refs_strong_ + refs_weak_; }

	// increase
	template <pointer_kind Kind>
	inline void ref_inc() {
		if constexpr( Kind == pk_strong ) ++refs_strong_;
		else ++refs_weak_;
	}
	void ref_inc(pointer_kind kind) { (kind == pk_strong) ? ref_inc<pk_strong>() : ref_inc<pk_weak>(); }

	// decrease
	template <pointer_kind Kind>
	inline bool ref_dec() {
		if constexpr( Kind == pk_strong ) {
			if( --refs_strong_ < 1 ) close();
		} else {
			--refs_weak_;
		}
		return refs_total();
	}
	bool ref_dec(pointer_kind kind) { return (kind == pk_strong) ? ref_inc<pk_strong>() : ref_inc<pk_weak>(); }
};

} // ns detail

// strong

template <typename T, template <typename T1> typename Closer>
struct pointer_strong :
	public bases::with_handle< detail::impl_pointer<T, Closer> >
{
	using type = T;
	using pointer = type *;
	using t_closer = Closer<type>;
	using t_impl = detail::impl_pointer<type, Closer>;
	using t_pointer = t_impl *;
	using base = bases::with_handle<t_impl>;
	static constexpr pointer_kind self_kind = pk_strong;

	~pointer_strong() { decrease(); }

	// copy
	pointer_strong(const pointer_strong & ptr) : base{ptr.h_} { increase(); }
	pointer_strong & operator = (const pointer_strong & ptr) {
		ptr.increase();
		decrease();
		this->h_ = ptr.h_;
		return *this;
	}

	//
	pointer_strong() : base{ new t_impl{1, 0} } {}
	pointer_strong(pointer obj) : pointer_strong() { this->h_->init_care(obj); }
	pointer_strong & operator = (pointer obj) {
		this->h_->change(obj);
		return *this;
	}

	t_pointer impl() const { return this->h_; }
	pointer data() const { return this->h_->h_; }
	pointer operator -> () const { return this->h_->h_; }
	operator bool () const { return this->h_->h_; }
	bool operator ! () const { return !this->h_->h_; }

private:
	inline void increase() { this->h_->template ref_inc<self_kind>(); }
	inline void decrease() { if( this->h_->template ref_dec<self_kind>() ) this->h_->deleter_(this->h_); }
};

// weak

template <typename T, template <typename T1> typename Closer>
struct pointer_weak :
	public bases::with_handle< detail::impl_pointer<T, Closer> >
{
	using t_strong = pointer_strong<T, Closer>;
	using type = t_strong::type;
	using pointer = t_strong::pointer;
	using t_closer = t_strong::t_closer;
	using t_impl = t_strong::t_impl;
	using t_pointer = t_strong::t_pointer;
	using base = bases::with_handle<t_impl>;
	static constexpr pointer_kind self_kind = pk_weak;

	// data
	t_pointer h_;

	~pointer_weak() { decrease(); }

	// copy
	pointer_weak(const pointer_weak & ptr) : base{ptr.h_} { increase(); }
	pointer_weak & operator = (const pointer_weak & ptr) {
		ptr.increase();
		decrease();
		this->h_ = ptr.h_;
		return *this;
	}

	// from strong
	pointer_weak(const t_strong & ptr) : base{ptr.h_} { increase(); }
	pointer_weak & operator = (const t_strong & ptr) {
		increase( ptr.impl() );
		decrease();
		this->h_ = ptr.h_;
		return *this;
	}

	//
	pointer_weak & operator = (pointer obj) {
		this->h_->change(obj);
		return *this;
	}

	t_pointer impl() const { return this->h_; }
	pointer data() const { return this->h_->h_; }
	pointer operator -> () const { return this->h_->h_; }
	operator bool () const { return this->h_->h_; }
	bool operator ! () const { return !this->h_->h_; }

private:
	static inline void increase(t_pointer h) { h->template ref_inc<self_kind>(); }
	inline void increase() { this->h_->template ref_inc<self_kind>(); }
	inline void decrease() { this->h_->template ref_dec<self_kind>(); }
};

} // ns just