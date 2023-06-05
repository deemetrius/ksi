module;

#include "../src/pre.h"

export module just.pool;

//export import just.list_forward;
//export import just.aligned;
//export import just.array;
//import <unique_ptr>;
export import just.std;
import <type_traits>;
import <memory>;
import <new>;

export namespace just {

	template <typename T>
	concept c_trivial_des = std::is_trivially_destructible_v<T>;

	template <typename T>
	struct mem {
		using t_element = T;
		using pointer = T *;

		static pointer alloc() { return new(std::nothrow) t_element; }
		static void free(pointer p) { delete p; }

		using t_alloc = decltype(&alloc);
		using t_free = decltype(&free);

		// data
		t_alloc
			m_alloc = &alloc;
		t_free
			m_free = &free;

		pointer do_alloc() const {
			pointer ret = m_alloc();
			if( ret == nullptr ) { throw std::bad_alloc{}; }
			return ret;
		}
	};

	template <c_trivial_des T, t_size Pack_size>
	struct pool {
		using t_element = T;
		using t_pointer = T *;

		static constexpr t_size s_pack_size = Pack_size;

		union t_data {
			empty_type	m_empty;
			t_element	m_target;

			t_data() {}
		};

		struct t_pack {
			using pointer = t_pack *;

			// data
			t_data
				m_data[s_pack_size];
			t_size
				m_index = 0;
			pointer
				m_next_pack;

			bool full() const { return m_index >= s_pack_size; }
			t_pointer get() { return &m_data[m_index].m_target; }
		};

		using t_mem = mem<t_pack>;
		using t_mem_const_pointer = const t_mem *;
		using pack_pointer = t_pack *;

		static constexpr t_mem s_mem;

		// data
		t_pack
			m_pack;
		pack_pointer
			m_current = &m_pack;
		t_mem_const_pointer
			m_mem = &s_mem;

		~pool() { clear(); }

		template <typename ... Args>
		t_pointer make(Args && ... p_args) {
			if( m_current->full() ) {
				pack_pointer p = m_mem->do_alloc();
				p->m_next_pack = m_current;
				m_current = p;
			}
			t_pointer ret = std::construct_at(m_current->get(), std::forward<Args>(p_args) ...);
			++m_current->m_index;
			return ret;
		}

		void clear() {
			pack_pointer p;
			while( m_current != &m_pack ) {
				p = m_current->m_next_pack;
				m_mem->m_free(m_current);
				m_current = p;
			}
			m_pack.m_index = 0;
		}
	};

} // ns