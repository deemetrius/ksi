module;

export module just.array;
export import just.ref;
import <concepts>;
import <type_traits>;
import <cstring>;
import <exception>;
import <new>;

export namespace just {

namespace closers {

namespace bases {

template <typename Closer>
struct with_close_range {
	template <typename Range>
	static void close_range(const Range & r) { for( auto & it : r ) Closer::close(&it); }
};

} // ns bases

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
	using const_pointer = const type *;
	using pointer = type *;

	enum : uid { item_size = sizeof(type) };
	static constexpr std::align_val_t v_align{alignof(type)};

	static pointer allocate(id capacity) { return reinterpret_cast<pointer>(new(v_align) char[capacity * item_size]); }
	static void deallocate(pointer h) { ::operator delete [] (reinterpret_cast<char *>(h), v_align); }
	static void close(pointer h) {}

	using tfn_deallocator = decltype(&deallocate);
};

struct impl_array_base {
	id capacity_, count_ = 0;
};

template <typename T>
struct impl_array :
	public ref<T, traits_ref_unique<impl_array_allocator> >,
	public impl_array_base
{
	using type = T;
	using const_pointer = const type *;
	using pointer = type *;
	using t_allocator = impl_array_allocator<type>;
	using t_ref = ref<T, traits_ref_unique<impl_array_allocator> >;

	// data
	t_allocator::tfn_deallocator deallocator_ = t_allocator::deallocate;

	impl_array(id capacity) : t_ref( t_allocator::allocate(capacity) ), impl_array_base{capacity} {}
	~impl_array() { deallocator_(this->h_); }

	// no copy
	impl_array(const impl_array &) = delete;
	impl_array & operator = (const impl_array &) = delete;

	friend void swap(impl_array & r1, impl_array & r2) {
		std::ranges::swap( static_cast<impl_array_base &>(r1), static_cast<impl_array_base &>(r2) );
		std::ranges::swap( static_cast<t_ref &>(r1), static_cast<t_ref &>(r2) );
		std::ranges::swap(r1.deallocator_, r2.deallocator_);
	}

	// range-for helpers
	using t_range = range<pointer>;
	using t_reverse_iterator = reverse_iterator<pointer>;
	using t_reverse_range = range<t_reverse_iterator>;

	t_range get_range() const { return {this->h_, this->h_ + count_}; }
	t_range get_range(id from) const { return {this->h_ + from, this->h_ + count_}; }
	t_range get_range(id from, id cnt) const { pointer it = this->h_ + from; return {it, it + cnt}; }

	t_reverse_range get_reverse_range() const { pointer it = this->h_ -1; return {it + count_, it}; }
	t_reverse_range get_reverse_range(id from) const { pointer it = this->h_ -1; return {it + count_, it + from}; }
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

	t_ref ref_;

	array(id capacity = t_capacity::initial) : ref_( new t_impl(capacity) ) {}

	t_impl::base & base() const { return *ref_.h_; }
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

template <typename Array>
struct array_insert_guard :
	public array_add_guard<Array>
{
private:
	int uncaught_;
	const Array * target_;
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
		from_(res_ ? res_.capacity_ : n)
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
				std::memcpy(from_.data(), src, Array::stored_bytes(pos_) );
				src += pos_;
			}
			typename Array::pointer dest = this->place + this->quantity;
			std::memcpy(dest, src, Array::stored_bytes(rest) );
			(*target_)->count_ = 0;
			from_->count_ = new_count_;
			std::ranges::swap( target_->base(), from_.base() );
		} else {
			// in-place
			typename Array::pointer src = target_->data() + pos_, dest = src + this->quantity;
			std::memmove(dest, src, Array::stored_bytes(rest) );
			std::memcpy(src, this->place, Array::stored_bytes(this->quantity) );
			from_->count_ = 0;
			(*target_)->count_ = new_count_;
		}
	}
};

template <typename T>
constexpr T max(T a, T b) { return (a < b) ? b : a; }

} // ns detail

template <typename Array>
struct array_append_guard :
	public detail::array_add_guard<Array>
{
private:
	int uncaught_;
	const Array * target_;
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
			Array from(res.capacity_);
			if( to ) {
				// copy data
				std::memcpy(from.data(), to.data(), to.stored_bytes() );
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

template <typename Array>
struct array_insert_guard {
private:
	using t_append = array_append_guard<Array>;
	using t_insert = detail::array_insert_guard<Array>;
	enum : uid {
		local_size = detail::max(sizeof(t_append), sizeof(t_insert) ),
		local_align = detail::max(alignof(t_append), alignof(t_insert) )
	};
	alignas(local_align)
	char local_data_[local_size];

public:
	using t_add = detail::array_add_guard<Array>;

	const t_add * impl() const { return reinterpret_cast<const t_add *>(local_data_); }
	const t_add * operator -> () const { return impl(); }

	array_insert_guard(Array & to, id pos, const id n = 1) {
		if( pos >= to->count_ ) {
			new(local_data_) t_append(to, n);
		} else {
			new(local_data_) t_insert(to, pos, n);
		}
	}
	~array_insert_guard() { impl()->~t_add(); }
};

/*template <typename Array>
auto array_append_n(Array & to, id n) -> Array::pointer {
	id new_count;
	result_capacity_more res = Array::t_capacity::more(to.impl(), n, new_count);
	// in-place
	if( !res ) {
		id old_count = to->count_;
		to->count_ = new_count;
		return to.data() + old_count;
	}
	Array from(res.capacity_);
	// was empty
	if( !to ) {
		from->count_ = new_count;
		std::ranges::swap( to.base(), from.base() );
		return to.data();
	}
	// copy data
	std::memcpy(from.data(), to.data(), to.stored_bytes() );
	id old_count = to->count_;
	to->count_ = 0;
	from->count_ = new_count;
	std::ranges::swap( to.base(), from.base() );
	return to.data() + old_count;
}

template <typename Array>
auto array_insert_n(Array & to, id pos, id n) -> Array::pointer {
	if( pos >= to->count_ ) return array_append_n(to, n);
	id new_count;
	result_capacity_more res = Array::t_capacity::more(to.impl(), n, new_count);
	id rest = to->count_ - pos;
	// in-place
	if( !res ) {
		typename Array::pointer src = to.data() + pos, dest = src + n;
		if( rest ) std::memmove(dest, src, Array::stored_bytes(rest) );
		to->count_ = new_count;
		return src;
	}
	Array from(res.capacity_);
	if( pos ) std::memcpy(from.data(), to.data(), Array::stored_bytes(pos) );
	typename Array::pointer ret = from.data() + pos, dest = ret + n;
	if( rest ) {
		typename Array::pointer src = to.data() + pos;
		std::memcpy(dest, src, Array::stored_bytes(rest) );
	}
	to->count_ = 0;
	from->count_ = new_count;
	std::ranges::swap( to.base(), from.base() );
	return ret;
}*/

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
		std::memmove(dest, dest + n, Array::stored_bytes(n) );
		to->count_ -= n;
	} else {
		detail::impl_array_remove_till_end(to, pos);
	}
}

template <typename Array>
void array_clear(Array & to) {
	if constexpr( Array::is_special ) {
		Array::t_impl::t_range_closer::close_range( to->get_reverse_range() );
	}
	to->count_ = 0;
}

template <typename Array>
void array_reset(Array & to, id capacity = Array::t_capacity::initial) {
	to.ref_ = new Array::t_impl(capacity);
}

} // ns just