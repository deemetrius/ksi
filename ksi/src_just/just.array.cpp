module;

export module just.array;
export import just.ref;
import <cstring>;
import <iostream>;

export namespace just {

namespace closers {

/*template <typename T>
struct range_none {
	using type = T;
	using pointer = type *;
	using const_pointer = const type *;

	static void close_one(const_pointer h) {}

	template <typename Range>
	static void close_range(const Range & r) {}
};*/

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
struct impl_array_closer {
	using type = T;
	using const_pointer = const type *;
	using pointer = type *;

	enum : uid { item_size = sizeof(type) };

	static pointer allocate(id capacity) { return reinterpret_cast<pointer>(new char[capacity * item_size]); }
	static void close(const_pointer h) { delete [] reinterpret_cast<const char *>(h); }
};

struct impl_array_base {
	id capacity_, count_ = 0;
};

template <typename T>
struct impl_array :
	public bases::with_ref_count,
	public impl_array_base,
	public ref<T, traits_ref_unique<impl_array_closer> >,
	public bases::with_deleter< impl_array<T> >
{
	using type = T;
	using const_pointer = const type *;
	using pointer = type *;
	using t_closer = impl_array_closer<type>;
	using t_ref = ref<T, traits_ref_unique<impl_array_closer> >;

	impl_array(id capacity) : impl_array_base{capacity}, t_ref( t_closer::allocate(capacity) ) {}

	// no copy
	impl_array(const impl_array &) = delete;
	impl_array & operator = (const impl_array &) = delete;

	//t_ref & get_ref() { return *this; }
};

} // ns detail

struct result_capacity_more {
	id count_;
	bool flag_ = false;
	id capacity_;

	operator bool () const { return flag_; }
	bool operator ! () const { return !flag_; }
};

template <id Initial, id Step>
struct capacity_step {
	enum : id { initial = Initial, step = Step };

	static result_capacity_more more(const detail::impl_array_base * impl, id n) {
		id new_count = impl->count_ + n;
		if( new_count <= impl->capacity_ ) return {new_count};
		return {new_count, true, new_count + step};
	}
};

template <typename T, typename Capacity>
struct array_pod {
	using type = T;
	using pointer = type *;
	using t_capacity = Capacity;
	using t_impl = detail::impl_array<type>;
	using t_ref = ref<t_impl,
		traits_ref_cnt<false, closers::compound_cnt<false, closers::simple_call_deleter>::closer>
	>;
	
	friend void swap(array_pod & r1, array_pod & r2) { swap(r1.ref_, r2.ref_); }

	t_ref ref_;

	array_pod(id capacity = t_capacity::initial) : ref_( new t_impl(capacity) ) {}

	t_impl * impl() const { return ref_.h_; }
	t_impl * operator -> () const { return impl(); }
	id count() const { return impl()->count_; }
	bool empty() const { return count(); }
	operator bool () const { return empty(); }
	bool operator ! () const { return !empty(); }
	pointer data() const { return impl()->h_; }
	T & operator [] (id pos) const { return data()[pos]; }
	static id stored_bytes(id cnt) { return cnt * t_impl::t_closer::item_size; }
	id stored_bytes() const { return stored_bytes( count() ); }

	using t_range = range<pointer>;
	using t_reverse_iterator = reverse_iterator<pointer>;
	using t_reverse_range = range<t_reverse_iterator>;

	t_range get_range() const { pointer it = data(); return {it, it + count()}; }
	t_range get_range(id from) const { pointer it = data(); return {it + from, it + count()}; }
	t_range get_range(id from, id cnt) const { pointer it = data() + from; return {it, it + cnt}; }

	t_reverse_range get_reverse_range() const { pointer it = data() -1; return {it + count(), it}; }
	t_reverse_range get_reverse_range(id from) const { pointer it = data() -1; return {it + count(), it + from}; }
	t_reverse_range get_reverse_range(id from, id cnt) const { pointer it = data() + from -1; return {it + cnt, it}; }
};

template <typename Base, template <typename T1> typename Range_closer>
struct array_with_closer :
	public Base
{
	using base = Base;
	using type = base::type;
	using t_closer = Range_closer<type>;

	friend void swap(array_with_closer & r1, array_with_closer & r2) { swap(r1.ref_, r2.ref_); }

	using base::base;

	~array_with_closer() { t_closer::close_range( this->get_reverse_range() ); }
};

template <typename T, typename Capacity, template <typename T1> typename Closer>
using array = array_with_closer<array_pod<T, Capacity>, closers::range_compound<Closer>::template closer>;

template <typename Array>
auto append_n(Array & to, id n) -> Array::pointer {
	result_capacity_more res = Array::t_capacity::more(to.impl(), n);
	// in-place
	if( !res ) {
		id old_count = to->count_;
		to->count_ = res.count_;
		return to.data() + old_count;
	}
	Array from(res.capacity_);
	// was empty
	if( !to ) {
		from->count_ = res.count_;
		to = from;
		return to.data();
	}
	// copy data
	std::memcpy(from.data(), to.data(), to.stored_bytes() );
	id old_count = to->count_;
	to->count_ = 0;
	from->count_ = res.count_;
	to = from;
	return to.data() + old_count;
}

template <typename Array>
auto insert_n(Array & to, id pos, id n) -> Array::pointer {
	if( pos >= to->count_ ) return append_n(to, n);
	result_capacity_more res = Array::t_capacity::more(to.impl(), n);
	id rest = to->count_ - pos;
	// in-place
	if( !res ) {
		typename Array::pointer src = to.data() + pos, dest = src + n;
		if( rest ) std::memmove(dest, src, Array::stored_bytes(rest) );
		to->count_ = res.count_;
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
	from->count_ = res.count_;
	to = from;
	return ret;
}

} // ns just