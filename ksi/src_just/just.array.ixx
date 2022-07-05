module;

#include "../src/pre.h"

export module just.array;

import <type_traits>;
import <cstring>;
export import just.common;
export import just.ref;

export namespace just {

	using t_count = t_diff;

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

			static pointer allocate(t_count p_capacity) {
				return reinterpret_cast<pointer>( new raw[p_capacity] );
			}

			static void deallocate(pointer p_handle) {
				delete [] reinterpret_cast<raw_pointer>(p_handle);
			}

			using t_deallocator = decltype(&deallocate);
		};

		struct impl_array_base {
			// data
			t_count		m_capacity, m_count = 0, m_from = 0;
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

			// data
			//t_count		m_desired_next_capacity = 0;

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

		template <typename T>
		struct impl_array_simple :
			public impl_array<T>,
			public bases::with_deleter<impl_array_simple<T> *>,
			public bases::with_ref_count
		{
			using base = impl_array<T>;

			using base::base;
		};

		template <typename T, template <typename T1> typename T_closer>
		struct impl_array_special :
			public impl_array<T>,
			public bases::with_deleter<impl_array_special<T, T_closer> *>,
			public bases::with_ref_count
		{
			using t_closer = T_closer<T>;
			using base = impl_array<T>;

			using base::base;

			template <typename T_range>
			static void close_range(const T_range & p_range) {
				for( auto & v_it : p_range ) t_closer::close(v_it);
			}

			~impl_array_special() { close_range( this->get_reverse_range(this->m_from) ); }
		};

	} // ns
	
	struct result_capacity_more {
		// data
		bool		m_need_realloc = false;
		t_count		m_capacity;

		operator bool () const { return m_need_realloc; }
		bool operator ! () const { return !m_need_realloc; }
	};

	template <t_count C_initial, t_count C_step>
	struct capacity_step {
		enum : t_count { s_initial = C_initial, s_step = C_step };

		static result_capacity_more more(const detail::impl_array_base * p_impl,
			t_count p_more, t_count & p_new_count
		) {
			p_new_count = p_impl->m_count + p_more;
			if( p_new_count <= p_impl->m_capacity ) return {};
			return {true, p_new_count + s_step};
		}
	};

	template <
		typename T,
		typename T_capacity,
		template <typename T1> typename T_closer = closers::simple_none
	>
	struct array {
		using type = T;
		using pointer = type *;
		using t_capacity = T_capacity;

		static constexpr bool s_is_special = ! std::is_same_v<T_closer<type>,
			closers::simple_none<type>
		>;

		using t_impl = std::conditional_t<
			s_is_special,
			detail::impl_array_special<type, T_closer>,
			detail::impl_array_simple<type>
		>;
		using t_ref = ref<t_impl,
			ref_traits_count<false,
				closers::compound_count<false,
					closers::compound_call_deleter<false>::template t_closer
				>::template t_closer
			>
		>;

		// data
		t_ref	m_ref;

		array(t_count p_capacity = t_capacity::s_initial) : m_ref{new t_impl(p_capacity)} {}

		pointer begin() const { return m_ref->m_handle; }
		pointer end() const { return m_ref->m_handle + m_ref->m_count; }

		friend void swap(array & p_1, array & p_2) { std::ranges::swap(p_1.m_ref, p_2.m_ref); }

		void clear() {
			if constexpr( s_is_special ) {
				m_ref->close_range(m_ref->get_reverse_range() );
			}
			m_ref->m_count = 0;
		}

		t_count size() const { return m_ref->m_count; }
		t_impl::base_data & base() const { return *m_ref.m_handle; }
		t_impl * impl() const { return m_ref.m_handle; }
		t_impl * operator -> () const { return m_ref.m_handle; }
		operator bool () const { return m_ref->m_count; }
		bool operator ! () const { return !m_ref->m_count; }
		pointer data() const { return m_ref->m_handle; }
		T & operator [] (t_count p_index) const { return m_ref->m_handle[p_index]; }
	};

	template <typename T, typename T_capacity>
	using array_alias = std::conditional_t<std::is_trivially_destructible_v<T>,
		array<T, T_capacity>,
		array<T, T_capacity, closers::simple_destructor>
	>;

	template <typename T_array, typename ... T_args>
	void array_append(T_array & p_array, T_args && ... p_args) {
		t_count v_new_count;
		if( result_capacity_more v_res = T_array::t_capacity::more(p_array.impl(), 1, v_new_count) ) {
			T_array v_array(v_res.m_capacity);
			new( v_array.data() + p_array->m_count ) T_array::type{std::forward<T_args>(p_args) ...};
			if( p_array ) {
				std::memcpy(v_array.data(), p_array.data(), sizeof(T_array::type) * p_array->m_count);
				p_array->m_count = 0;
			}
			v_array->m_count = v_new_count;
			std::ranges::swap( p_array.base(), v_array.base() );
		} else {
			new( p_array.end() ) T_array::type{std::forward<T_args>(p_args) ...};
			p_array->m_count = v_new_count;
		}
	}

	template <typename T_array, typename ... T_args>
	void array_append_n(T_array & p_array, t_count p_count, const T_args & ... p_args) {
		t_count v_new_count;
		if( result_capacity_more v_res = T_array::t_capacity::more(p_array.impl(), p_count, v_new_count) ) {
			T_array v_array(v_res.m_capacity);
			v_array->m_from = p_array->m_count;
			v_array->m_count = p_array->m_count;
			for( t_count v_index = p_array->m_count; v_index < v_new_count; ++v_index ) {
				new( v_array.data() + v_index ) T_array::type{p_args ...};
				++v_array->m_count;
			}
			if( p_array ) {
				std::memcpy(v_array.data(), p_array.data(), sizeof(T_array::type) * p_array->m_count);
				p_array->m_count = 0;
				v_array->m_from = 0;
			}
			std::ranges::swap( p_array.base(), v_array.base() );
		} else {
			for( t_count v_index = p_array->m_count; v_index < v_new_count; ++v_index ) {
				new( p_array.data() + v_index ) T_array::type{p_args ...};
				++p_array->m_count;
			}
		}
	}

	template <typename T_array>
	void array_remove_from_end(T_array & p_array, t_count p_count_from_end) {
		t_count v_from = p_array->m_count - p_count_from_end;
		if constexpr( T_array::s_is_special ) {
			T_array::t_impl::close_range( p_array->get_reverse_range(v_from) );
		}
		p_array->m_count = v_from;
	}

	template <typename T_array, typename ... T_args>
	void array_insert(T_array & p_array, t_count p_position, T_args && ... p_args) {
		if( p_position >= p_array->m_count ) {
			array_append(p_array, std::forward<T_args>(p_args) ...);
			return;
		}
		aligned_as<T_array::type> v_data;
		new(&v_data) T_array::type{std::forward<T_args>(p_args) ...};
		t_count v_new_count, v_rest = p_array->m_count - p_position;
		if( result_capacity_more v_res = T_array::t_capacity::more(p_array.impl(), 1, v_new_count) ) {
			T_array v_array{v_res.m_capacity};
			if( p_position ) {
				std::memcpy(v_array.data(), p_array.data(), sizeof(T_array::type) * p_position);
			}
			std::memcpy(v_array.data() + p_position, &v_data, sizeof(T_array::type) );
			std::memcpy(v_array.data() + p_position +1, p_array.data() + p_position,
				sizeof(T_array::type) * v_rest
			);
			v_array->m_count = v_new_count;
			p_array->m_count = 0;
			std::ranges::swap(p_array.base(), v_array.base() );
		} else {
			std::memmove(p_array.data() + p_position +1, p_array.data() + p_position,
				sizeof(T_array::type) * v_rest
			);
			std::memcpy(p_array.data() + p_position, &v_data, sizeof(T_array::type) );
			p_array->m_count = v_new_count;
		}
	}

	template <typename T_array>
	void array_remove(T_array & p_array, t_count p_from, t_count p_count = 1) {
		if constexpr( T_array::s_is_special ) {
			T_array::t_impl::close_range( p_array->get_reverse_range(p_from, p_count) );
		}
		t_count v_rest = p_array->m_count - p_from - p_count;
		if( v_rest ) {
			std::memmove(p_array.data() + p_from, p_array.data() + p_from + p_count,
				sizeof(T_array::type) * v_rest
			);
		}
		p_array->m_count -= p_count;
	}

} // ns