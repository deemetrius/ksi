module;

#include "../src/pre.h"

export module just.array;

export import just.common;
export import just.ref;

export namespace just {
	
	/*template <t_size C_size, t_size C_align>
	struct alignas(C_align) aligned_data {
		using type = t_byte;
		enum : t_size { s_size = C_size, s_align = C_align };
		
		// data
		type m_data[N];
	};
	
	template <typename T>
	using aligned_as = aligned_data<sizeof(T), alignof(T)>;*/
	
	namespace detail {
		
		template <typename T>
		struct impl_array :
			public bases::with_deleter<T, closers::simple_delete_array>,
			public bases::with_handle<T>,
			public bases::with_ref_count
		{
			using type = T;
			using t_with_handle = bases::with_handle<T>;
			using t_count = t_int_max;
			//using t_item = aligned_as<type>;
			//using t_item_pointer = t_item *;
			
			// data
			t_count		m_count = 0;
			
			impl_array(t_count p_count) :
				t_with_handle{ new type[p_count] },
				m_count(p_count)
			{}
			
			impl_array() = default;
			~impl_array() { this->m_deleter(this->m_handle); }
			
			// no copy
			impl_array(const impl_array &) = delete;
			impl_array & operator = (const impl_array &) = delete;
		};
		
	} // ns
	
	template <typename T>
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
	};
	
} // ns