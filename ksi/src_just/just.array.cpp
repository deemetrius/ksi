module;

export module just.array;
export import just.ref;
import <concepts>;
import <type_traits>;
import <cstring>;
import <exception>;
import <new>;

export namespace just {

namespace bases {

template <typename Closer>
struct with_close_range {
	template <typename Range>
	static void close_range(const Range & r) { for( auto & it : r ) Closer::close(&it); }
};

} // ns bases

namespace closers {

template <template <typename T1> typename Closer>
struct range_compound {
	template <typename T>
	struct closer :
		public Closer<T>,
		public bases::with_close_range< Closer<T> >
	{};
};

} // ns closers

namespace detail {

template <typename T>
struct impl_array_allocator {
	using type = T;
	using pointer = type *;

	enum : uid { item_size = sizeof(type) };
	static constexpr std::align_val_t v_align{alignof(type)};

	static pointer allocate(id capacity) { return reinterpret_cast<pointer>(new(v_align) char[capacity * item_size]); }
	static void deallocate(pointer h) { ::operator delete [] (reinterpret_cast<char *>(h), v_align); }

	using tfn_deallocator = decltype(&deallocate);
};

struct impl_array_base {
	id capacity_, count_ = 0;
};

template <typename T>
struct impl_array_base_ex :
	public impl_array_base
{
	using t_allocator = impl_array_allocator<T>;
	using pointer = T *;

	// data
	pointer h_;
	t_allocator::tfn_deallocator deallocator_ = t_allocator::deallocate;

	impl_array_base_ex(pointer h, id capacity) : impl_array_base{capacity}, h_{h} {}
};

template <typename T>
struct impl_array :
	public impl_array_base_ex<T>
{
	using type = T;
	using pointer = type *;
	using const_pointer = const type *;
	using base_ex = impl_array_base_ex<type>;
	using typename base_ex::t_allocator;

	// data
	id desired_next_capacity_ = 0;

	// no copy
	impl_array(const impl_array &) = delete;
	impl_array & operator = (const impl_array &) = delete;

	//
	impl_array(id capacity) : base_ex(t_allocator::allocate(capacity), capacity) {}
	~impl_array() { this->deallocator_(this->h_); }

	// range-for helpers
	using t_range = range<pointer>;
	using t_reverse_iterator = reverse_iterator<pointer>;
	using t_reverse_range = range<t_reverse_iterator>;

	t_range get_range() const { return {this->h_, this->h_ + this->count_}; }
	t_range get_range(id from) const { return {this->h_ + from, this->h_ + this->count_}; }
	t_range get_range(id from, id cnt) const { pointer it = this->h_ + from; return {it, it + cnt}; }

	t_reverse_range get_reverse_range() const { pointer it = this->h_ -1; return {it + this->count_, it}; }
	t_reverse_range get_reverse_range(id from) const { pointer it = this->h_ -1; return {it + this->count_, it + from}; }
	t_reverse_range get_reverse_range(id from, id cnt) const { pointer it = this->h_ + from -1; return {it + cnt, it}; }
};

template <typename T>
struct impl_array_simple :
	public impl_array<T>,
	public bases::with_deleter< impl_array_simple<T> >,
	public bases::with_ref_count
{
	using base = impl_array<T>;

	using base::base;
};

template <typename T, template <typename T1> typename Range_closer>
struct impl_array_special :
	public impl_array<T>,
	public bases::with_deleter< impl_array_special<T, Range_closer> >,
	public bases::with_ref_count
{
	using t_range_closer = Range_closer<T>;
	using base = impl_array<T>;

	using base::base;

	~impl_array_special() { t_range_closer::close_range( this->get_reverse_range() ); }
};

} // ns detail

// capacity

struct result_capacity_more {
	bool flag_ = false;
	id capacity_;

	operator bool () const { return flag_; }
	bool operator ! () const { return !flag_; }
};

template <id Initial, id Step>
struct capacity_step {
	enum : id { initial = Initial, step = Step };

	static result_capacity_more more(const detail::impl_array_base * impl, id n, id & new_count) {
		new_count = impl->count_ + n;
		if( new_count <= impl->capacity_ ) return {};
		return {true, new_count + step};
	}
};

template <typename T>
concept c_capacity_more = c_any_of<T, case_default, case_exact>;

// array

template <typename T, typename Capacity, template <typename T1> typename Closer = closers::simple_none>
struct array {
	using type = T;
	using pointer = type *;
	using t_capacity = Capacity;
	static constexpr bool is_special = ! std::is_same_v<Closer<T>, closers::simple_none<T> >;
	using t_impl = std::conditional_t<
		is_special,
		detail::impl_array_special<type, closers::range_compound<Closer>::template closer>,
		detail::impl_array_simple<type>
	>;
	using t_ref = ref<t_impl,
		traits_ref_cnt<false, closers::compound_cnt<false, false, closers::simple_call_deleter>::closer>
	>;
	
	friend void swap(array & r1, array & r2) { std::ranges::swap(r1.ref_, r2.ref_); }

	// data
	t_ref ref_;

	//
	array(id capacity = t_capacity::initial) : ref_( new t_impl(capacity) ) {}

	t_impl::base_ex & base() const { return *ref_.h_; }
	t_impl * impl() const { return ref_.h_; }
	t_impl * operator -> () const { return ref_.h_; }
	operator bool () const { return ref_->count_; }
	bool operator ! () const { return !ref_->count_; }
	pointer data() const { return ref_->h_; }
	T & operator [] (id pos) const { return ref_->h_[pos]; }
	static id stored_bytes(id cnt) { return cnt * t_impl::t_allocator::item_size; }
	id stored_bytes() const { return stored_bytes(ref_->count_); }
};

// actions

namespace detail {

template <typename Array>
struct array_add_guard {
	const id quantity;
	Array::pointer place;

	array_add_guard(const id n) : quantity(n) {}
	virtual ~array_add_guard() = default;
};

template <c_capacity_more Case_capacity_more>
constexpr id choose_capacity(id proposed, id exact) {
	if constexpr( std::is_same_v<Case_capacity_more, case_exact> ) { return exact; }
	else { return proposed; }
}

template <c_capacity_more Case_capacity_more>
constexpr id choose_capacity(id proposed, id exact, id desired) {
	return (desired > exact) ? desired : choose_capacity<Case_capacity_more>(proposed, exact);
}

// detail insert

template <typename Array, c_capacity_more Case_capacity_more>
struct array_insert_guard :
	public array_add_guard<Array>
{
private:
	int uncaught_;
	Array * target_;
	id new_count_, pos_;
	result_capacity_more res_;
	Array from_;

public:
	using base = array_add_guard<Array>;

	array_insert_guard(Array & to, id pos, const id n) :
		base(n),
		uncaught_( std::uncaught_exceptions() ),
		target_(&to),
		pos_(pos),
		res_( Array::t_capacity::more(to.impl(), n, new_count_) ),
		from_(res_ ? choose_capacity<Case_capacity_more>(
			res_.capacity_, new_count_, to->desired_next_capacity_
		) : n)
	{
		this->place = from_.data();
		if( res_ ) this->place += pos;
	}
	~array_insert_guard() override {
		if( uncaught_ < std::uncaught_exceptions() ) return;
		id rest = (*target_)->count_ - pos_;
		if( res_ ) {
			// more amount_
			typename Array::pointer src = target_->data();
			if( pos_ ) {
				std::memcpy(reinterpret_cast<char *>( from_.data() ), src, Array::stored_bytes(pos_) );
				src += pos_;
			}
			typename Array::pointer dest = this->place + this->quantity;
			std::memcpy(reinterpret_cast<char *>( dest ), src, Array::stored_bytes(rest) );
			(*target_)->count_ = 0;
			from_->count_ = new_count_;
			std::ranges::swap( target_->base(), from_.base() );
		} else {
			// in-place
			typename Array::pointer src = target_->data() + pos_, dest = src + this->quantity;
			std::memmove(reinterpret_cast<char *>( dest ), src, Array::stored_bytes(rest) );
			std::memcpy(reinterpret_cast<char *>( src ), this->place, Array::stored_bytes(this->quantity) );
			from_->count_ = 0;
			(*target_)->count_ = new_count_;
		}
	}
};

} // ns detail

// append

template <typename Array, c_capacity_more Case_capacity_more = case_default>
struct array_append_guard :
	public detail::array_add_guard<Array>
{
private:
	int uncaught_;
	Array * target_;
	id new_count_;

public:
	using base = detail::array_add_guard<Array>;

	array_append_guard(Array & to, const id n = 1) :
		base(n),
		uncaught_( std::uncaught_exceptions() ),
		target_(&to)
	{
		if( result_capacity_more res = Array::t_capacity::more(to.impl(), n, new_count_) ) {
			// more amount_
			const id new_capacity = detail::choose_capacity<Case_capacity_more>(
				res.capacity_, new_count_, to->desired_next_capacity_
			);
			Array from(new_capacity);
			if( to ) {
				// copy data
				std::memcpy(reinterpret_cast<char *>( from.data() ), to.data(), to.stored_bytes() );
				from->count_ = to->count_;
				to->count_ = 0;
			}
			std::ranges::swap( to.base(), from.base() );
		}
		this->place = to.data() + to->count_;
	}
	~array_append_guard() override {
		if( uncaught_ >= std::uncaught_exceptions() ) (*target_)->count_ = new_count_;
	}
};

// insert

template <typename Array, c_capacity_more Case_capacity_more = case_default>
struct array_insert_guard {
	using t_add = detail::array_add_guard<Array>;
	using pointer = t_add *;
	using const_pointer = const t_add *;

private:
	using t_append = array_append_guard<Array, Case_capacity_more>;
	using t_insert = detail::array_insert_guard<Array, Case_capacity_more>;
	enum : uid {
		local_size = max(sizeof(t_append), sizeof(t_insert) ),
		local_align = max(alignof(t_append), alignof(t_insert) )
	};

	// data
	alignas(local_align)
	aligned_data<local_size, local_align> local_data_;
	pointer h_;

public:
	const_pointer impl() const { return h_; }
	const_pointer operator -> () const { return h_; }

	array_insert_guard(Array & to, id pos, const id n = 1) {
		if( pos >= to->count_ ) {
			h_ = new(&local_data_) t_append(to, n);
		} else {
			h_ = new(&local_data_) t_insert(to, pos, n);
		}
	}
	~array_insert_guard() { h_->~t_add(); }
};

// remove

namespace detail {

template <typename Array>
inline void impl_array_remove_till_end(Array & to, id pos) {
	if constexpr( Array::is_special ) {
		Array::t_impl::t_range_closer::close_range( to->get_reverse_range(pos) );
	}
	to->count_ = pos;
}

} // ns detail

template <typename Array>
void array_remove_till_end(Array & to, id pos) {
	detail::impl_array_remove_till_end(to, pos);
}

template <typename Array>
void array_remove_last_n(Array & to, id n = 1) {
	id pos = to->count_ - n;
	detail::impl_array_remove_till_end(to, pos);
}

template <typename Array>
void array_remove_n(Array & to, id pos, id n = 1) {
	if( pos + n < to->count_ ) {
		if constexpr( Array::is_special ) {
			Array::t_impl::t_range_closer::close_range( to->get_reverse_range(pos, n) );
		}
		typename Array::pointer dest = to.data() + pos;
		std::memmove(reinterpret_cast<char *>( dest ), dest + n, Array::stored_bytes(n) );
		to->count_ -= n;
	} else {
		detail::impl_array_remove_till_end(to, pos);
	}
}

// clear

template <typename Array>
void array_clear(Array & to) {
	if constexpr( Array::is_special ) {
		Array::t_impl::t_range_closer::close_range( to->get_reverse_range() );
	}
	to->count_ = 0;
}

// reset

template <typename Array>
void array_reset(Array & to, id capacity = Array::t_capacity::initial) {
	to.ref_ = new Array::t_impl(capacity);
}

} // ns just