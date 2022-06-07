module;

#include "../src/pre.h"

export module just.array;

export import just.common;
export import just.ref;

export namespace just {
	
	template <t_size C_size, t_size C_align>
	struct alignas(C_align) aligned_data {
		using type = t_byte_under;
		enum : t_size { s_size = C_size, s_align = C_align };
		
		// data
		type m_data[s_size];
	};
	
	template <typename T>
	using aligned_as = aligned_data<sizeof(T), alignof(T)>;
	
	namespace detail {
		
		template <typename T>
		struct array_allocator {
			using type = T;
			using pointer = type *;
			using raw = aligned_as<T>;
			using raw_pointer = raw *;
			using t_count = t_diff;

			static pointer allocate(t_count p_capacity) {
				return reinterpret_cast<pointer>( new raw[p_capacity] );
			}

			static void deallocate(pointer p_handle) {
				delete [] reinterpret_cast<raw_pointer>(p_handle);
			}

			using t_deallocator = decltype(&deallocate);
		};

		struct impl_array_base {
			using t_count = t_diff;

			// data
			t_count		m_capacity, m_count = 0;
		};

		template <typename T>
		struct impl_array_data :
			public impl_array_base
		{
			using type = T;
			using pointer = type *;
			using t_allocator = array_allocator<type>;

			// data
			pointer						m_handle;
			t_allocator::t_deallocator	m_deallocate = &t_allocator::deallocate;

			impl_array_data(pointer p_handle, t_count p_capacity) :
				impl_array_base{p_capacity}, m_handle{p_handle}
			{}
		};

		template <typename T>
		struct impl_array :
			public impl_array_data<T>
		{
			using type = T;
			using pointer = type *;
			using base_data = impl_array_data<type>;
			using typename base_data::t_allocator;
			using typename base_data::t_count;

			// data
			t_count		m_desired_next_capacity = 0;

			impl_array(t_count p_capacity) : base_data{t_allocator::allocate(p_capacity), p_capacity} {}
			~impl_array() { this->m_deallocate(this->m_handle); }

			// no copy, no move
			impl_array(const impl_array &) = delete;
			impl_array(impl_array &&) = delete;
			impl_array & operator = (const impl_array &) = delete;
			impl_array & operator = (impl_array &&) = delete;

			// range-for helpers
			using t_range = range_for<pointer>;
			using t_reverse_iterator = reverse_iterator<pointer>;
			using t_reverse_range = range_for<t_reverse_iterator, pointer>;

			t_range get_range() const {
				return {this->m_handle, this->m_handle + this->m_count};
			}
			t_range get_range(t_count p_from) const {
				return {this->m_handle + p_from, this->m_handle + this->m_count};
			}
			t_range get_range(t_count p_from, t_count p_count) const {
				pointer v_it = this->m_handle + p_from;
				return {v_it, v_it + p_count};
			}

			t_reverse_range get_reverse_range() const {
				pointer v_it = this->m_handle -1;
				return {v_it + this->m_count, v_it};
			}
			t_reverse_range get_reverse_range(t_count p_from) const {
				pointer v_it = this->m_handle -1;
				return {v_it + this->m_count, v_it + p_from};
			}
			t_reverse_range get_reverse_range(t_count p_from, t_count p_count) const {
				pointer v_it = this->m_handle + p_from -1;
				return {v_it + p_count, v_it};
			}
		};

	} // ns
	
	/*template <typename T>
	struct array {
		using type = T;
		using pointer = type *;
		using t_impl = detail::impl_array<type>;
		using t_count = t_impl::t_count;
		using t_ref = ref<t_impl, ref_traits_count<false,
			closers::compound_count<false>::template t_closer
		> >;
		
		// data
		t_ref	m_ref;
		
		array() : m_ref{ new t_impl } {}
		array(t_count p_count) : m_ref{ new t_impl{p_count} } {}
		
		void new_size(t_count p_count) { m_ref = new t_impl{p_count}; }
		
		operator bool () const { return m_ref->m_count; }
		bool operator ! () const { return !m_ref->m_count; }
		
		type & operator [] (t_count p_index) { return m_ref->m_handle[p_index]; }
		
		pointer begin() { return m_ref->m_handle; }
		pointer end() { return m_ref->m_handle + m_ref->m_count; }
	};*/
	
} // ns