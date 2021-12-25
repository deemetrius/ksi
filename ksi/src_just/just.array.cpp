module;

export module just.array;
export import just.ref;

export namespace just {

namespace detail {

template <typename T>
struct impl_array :
	public with_ref_count,
	public with_handle<T>,
	public with_deleter< impl_array<T> >
{
	using type = T;
	using const_pointer = const type *;
	using pointer = type *;
	enum : uid { item_size = sizeof(T) };

	static pointer allocate(id capacity) { return reinterpret_cast<pointer>(new char[capacity * item_size]); }
	static void free(const_pointer h) { delete [] reinterpret_cast<const char *>(h); }

	id count_ = 0, capacity_;

	impl_array(id capacity) : capacity_(capacity) { this->h_ = allocate(capacity); }
	~impl_array() { free(this->h_); }
};

} // ns detail

template <id Initial, id Step>
struct capacity_step {
	enum : id { initial = Initial, step = Step };

	static constexpr id more(id current_capacity, id new_capacity) { return new_capacity + step; }
};

template <typename T, typename Capacity>
struct array {
	using type = T;
	using pointer = type *;
	using t_capacity = Capacity;
	using t_impl = detail::impl_array<type>;
	using t_ref = ref<t_impl,
		traits_ref_cnt<false, closer_cnt_with<false, closer_deleter>::closer>
	>;
	
	t_ref ref_;

	array() : ref_( new t_impl(t_capacity::initial) ) {}

	t_impl * impl() const { return ref_.h_; }
	t_impl * operator -> () const { return impl(); }
	id count() const {return impl()->count_; }
	bool empty() const { return count(); }
	operator bool () const { return empty(); }
	pointer data() const { return impl()->h_; }
	T & operator [] (id pos) const { return data()[pos]; }

	using t_range = range<pointer>;
	using t_reverse_iterator = reverse_iterator<pointer>;
	using t_reverse_range = range<t_reverse_iterator>;

	t_range get_range() { pointer it = data(); return {it, it + count()}; }
	t_range get_range(id from) { pointer it = data(); return {it + from, it + count()}; }
	t_range get_range(id from, id cnt) { pointer it = data() + from; return {it, it + cnt}; }

	t_reverse_range get_reverse_range() { pointer it = data() -1; return {it + count(), it}; }
	t_reverse_range get_reverse_range(id from) { pointer it = data() -1; return {it + count(), it + from}; }
	t_reverse_range get_reverse_range(id from, id cnt) { pointer it = data() + from -1; return {it + cnt, it}; }
};

} // ns just