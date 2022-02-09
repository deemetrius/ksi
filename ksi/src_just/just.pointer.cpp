module;

export module just.pointer;
export import just.aux;

export namespace just {

enum class pointer_kind { weak, strong };

namespace detail {

// impl

template <typename T, template <typename T1> typename Closer>
struct impl_pointer {
	using type = T;
	using pointer = T *;
	using t_closer = Closer<T>;
	using t_deleter = bases::with_deleter< impl_pointer<T, closers::simple_one> >;
	using tfn_deleter = t_deleter::tfn_deleter;

	static constexpr pointer_kind
	kind_some = pointer_kind::strong,
	kind_other = pointer_kind::weak;

	// data
	id refs_strong_, refs_weak_;
	pointer h_ = nullptr;
	tfn_deleter deleter_ = t_deleter::t_closer::close;

	// no copy
	impl_pointer(const impl_pointer &) = delete;
	impl_pointer & operator = (const impl_pointer &) = delete;

	inline void close_only() {
		if constexpr( t_closer::can_accept_null ) { t_closer::close(h_); }
		else if( h_ ) { t_closer::close(h_); }
	}
	void close() { close_only(); h_ = nullptr; }
	inline void init_care(pointer obj) { h_ = obj; }
	void change_care(pointer obj) { close_only(); h_ = obj; }
	void change(pointer obj) { if( refs_strong_ ) change_care(obj); }

	id refs_total() const { return refs_strong_ + refs_weak_; }

	// increase
	template <pointer_kind Kind>
	inline void ref_inc() {
		if constexpr( Kind == pointer_kind::strong ) ++refs_strong_;
		else ++refs_weak_;
	}
	void ref_inc(pointer_kind kind) { (kind == kind_some) ? ref_inc<kind_some>() : ref_inc<kind_other>(); }

	// decrease
	template <pointer_kind Kind>
	inline bool ref_dec() {
		if constexpr( Kind == pointer_kind::strong ) {
			if( --refs_strong_ < 1 ) close();
		} else {
			--refs_weak_;
		}
		return refs_total();
	}
	bool ref_dec(pointer_kind kind) { return (kind == kind_some) ? ref_inc<kind_some>() : ref_inc<kind_other>(); }
};

// base

template <typename T, template <typename T1> typename Closer>
struct pointer_base {
	using type = T;
	using pointer = type *;
	using t_impl = detail::impl_pointer<type, Closer>;
	using t_pointer = t_impl *;

	// data
	t_pointer h_ = nullptr;

	t_pointer impl() const { return h_; }
	pointer data() const { return h_->h_; }
	pointer operator -> () const { return h_->h_; }
	type & operator * () const { return *h_->h_; }
	operator bool () const { return h_->h_; }
	bool operator ! () const { return !h_->h_; }
};

// base exact

template <typename T, template <typename T1> typename Closer, pointer_kind Kind>
struct pointer_exact :
	public pointer_base<T, Closer>
{
	using base = pointer_base<T, Closer>;
	using typename base::t_pointer;

	static constexpr pointer_kind self_kind = Kind;

	//
	~pointer_exact() { decrease(); }

protected:
	static inline void increase(t_pointer h) { h->template ref_inc<self_kind>(); }
	inline void increase() { this->h_->template ref_inc<self_kind>(); }
	inline void decrease() { if( this->h_->template ref_dec<self_kind>() ) this->h_->deleter_(this->h_); }
	inline void assign(t_pointer h) {
		increase(h);
		decrease();
		this->h_ = h;
	}
};

} // ns detail

// strong

template <typename T, template <typename T1> typename Closer>
struct pointer_strong :
	public detail::pointer_exact<T, Closer, pointer_kind::strong>
{
	using base_exact = detail::pointer_exact<T, Closer, pointer_kind::strong>;
	using typename base_exact::base;
	using typename base::type;
	using typename base::pointer;
	using typename base::t_impl;
	using typename base::t_pointer;

	// copy
	pointer_strong(const pointer_strong & ptr) : base_exact{ptr.h_} { this->increase(); }
	pointer_strong & operator = (const pointer_strong & ptr) { this->assign( ptr.impl() ); return *this; }

	// from other
	pointer_strong(const base & ptr) : base_exact{ptr.h_} { this->increase(); }
	pointer_strong & operator = (const base & ptr) { this->assign( ptr.h_ ); return *this; }

	//
	pointer_strong() : base_exact{ new t_impl{1, 0} } {}
	pointer_strong(pointer obj) : pointer_strong() { this->h_->init_care(obj); }
	pointer_strong & operator = (pointer obj) {
		this->h_->change_care(obj);
		return *this;
	}
};

// weak

template <typename T, template <typename T1> typename Closer>
struct pointer_weak :
	public detail::pointer_exact<T, Closer, pointer_kind::weak>
{
	using base_exact = detail::pointer_exact<T, Closer, pointer_kind::weak>;
	using typename base_exact::base;
	using typename base::type;
	using typename base::pointer;
	using typename base::t_impl;
	using typename base::t_pointer;

	// copy
	pointer_weak(const pointer_weak & ptr) : base_exact{ptr.h_} { this->increase(); }
	pointer_weak & operator = (const pointer_weak & ptr) { this->assign( ptr.impl() ); return *this; }

	// from other
	pointer_weak(const base & ptr) : base_exact{ptr.h_} { this->increase(); }
	pointer_weak & operator = (const base & ptr) { this->assign( ptr.h_ ); return *this; }

	//
	pointer_weak() = delete;
	pointer_weak & operator = (pointer obj) {
		this->h_->change(obj);
		return *this;
	}
};

// semi

template <typename T, template <typename T1> typename Closer>
struct pointer_semi :
	public detail::pointer_base<T, Closer>
{
	using base = detail::pointer_base<T, Closer>;
	using typename base::type;
	using typename base::pointer;
	using typename base::t_impl;
	using typename base::t_pointer;

	// data
	pointer_kind kind_ = pointer_kind::strong;

	~pointer_semi() { decrease(); }

	// copy
	pointer_semi(const pointer_semi & ptr) : base{ptr.h_} { increase(); }
	pointer_semi & operator = (const pointer_semi & ptr) { assign( ptr.impl() ); return *this; }

	// from other
	pointer_semi(const base & ptr) : base{ptr.h_} { increase(); }
	pointer_semi & operator = (const base & ptr) { assign( ptr.h_ ); return *this; }

	// set exact kind
	pointer_semi(pointer_kind kind, const base & ptr) : base{ptr.h_}, kind_{kind} { increase(); }
	pointer_semi & assign(pointer_kind kind, const base & ptr) {
		ptr.h_->ref_inc(kind);
		decrease();
		kind_ = kind;
		this->h_ = ptr.h_;
	}

	//
	pointer_semi() : base{ new t_impl{1, 0} } {}
	pointer_semi(pointer obj) : pointer_semi() { this->h_->init_care(obj); }
	pointer_semi & operator = (pointer obj) {
		this->h_->change(obj);
		return *this;
	}

protected:
	inline void increase(t_pointer h) { h->ref_inc(kind_); }
	inline void increase() { this->h_->ref_inc(kind_); }
	inline void decrease() { if( this->h_->ref_dec(kind_) ) this->h_->deleter_(this->h_); }
	inline void assign(t_pointer h) {
		increase(h);
		decrease();
		this->h_ = h;
	}
};

} // ns just