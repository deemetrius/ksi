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

		pointer do_alloc() {
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
		using pack_pointer = t_pack *;

		// data
		t_pack
			m_pack;
		pack_pointer
			m_current = &m_pack;
		t_mem
			m_mem;

		~pool() { clear(); }

		template <typename ... Args>
		t_pointer make(Args && ... p_args) {
			if( m_current->full() ) {
				pack_pointer p = m_mem.do_alloc();
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
				m_mem.free(m_current);
				m_current = p;
			}
			m_pack.m_index = 0;
		}
	};

	/*
	template <typename T, t_size C_bucket_size>
	struct pool {
		using self_pointer = pool *;
		using type = T;
		using pointer = type *;

		static constexpr t_size s_bucket_size = C_bucket_size;
		static constexpr bool s_need_destroy = ! std::is_trivially_destructible_v<type>;

		using t_aligned = with_aligned_data_as<type>;
		using t_fixed = array<t_aligned, s_bucket_size>;

		struct bucket :
			public t_fixed,
			public node_forward<bucket>
		{
			using self_pointer = bucket *;
			using t_node = node_forward<bucket>;
			using t_aligned_pointer = t_aligned *;

			typename t_fixed::reverse_range reverse(t_aligned_pointer p_next) { return {{p_next -1, this->before_first()}}; }
		};

		static void bucket_add(self_pointer p_pool) {
			typename bucket::self_pointer v_bucket = new bucket;
			p_pool->m_zero.forward_attach(v_bucket);
			p_pool->m_next = v_bucket->m_items;
		}

		static void bucket_destroy(bucket::self_pointer p_bucket) { delete p_bucket; }

		using t_adder = decltype(&bucket_add);
		using t_destroyer = decltype(&bucket_destroy);

		// data
		bucket::t_node				m_zero;
		bucket::t_aligned_pointer	m_next = nullptr;
		t_adder						m_adder = &bucket_add;
		t_destroyer					m_destroyer = &bucket_destroy;

		pool() { bucket_add(this); }
		~pool() { clear(); m_destroyer( m_zero.m_next->forward_target() ); }

		// no no
		pool(const pool &) = delete;
		pool(pool &&) = delete;
		pool & operator = (const pool &) = delete;
		pool & operator = (pool &&) = delete;

		void advance() {
			++m_next;
			if( m_next == bucket_last()->after_last() ) { m_adder(this); }
		}

		bucket::self_pointer bucket_last() { return m_zero.m_next->forward_target(); }

		void clear() {
			typename bucket::self_pointer v_bucket_last = bucket_last();
			typename bucket::t_node::t_forward_range v_range = v_bucket_last->forward_range();
			if constexpr( s_need_destroy ) {
				for( typename bucket::t_item & v_item : v_bucket_last->reverse(m_next) ) {
					reinterpret_cast<pointer>(v_item.m_aligned)->~type();
				}
				for( typename bucket::self_pointer v_bucket : v_range ) {
					for( typename bucket::t_item & v_item : v_bucket->reverse() ) {
						reinterpret_cast<pointer>(v_item.m_aligned)->~type();
					}
					m_destroyer(v_bucket);
				}
			} else {
				for( typename bucket::self_pointer v_bucket : v_range ) {
					m_destroyer(v_bucket);
				}
			}
			v_bucket_last->forward_reset();
			m_next = v_bucket_last->m_items;
		}

		template <typename ... V_args>
		pointer emplace(V_args && ... p_args) {
			pointer ret = new(m_next->m_aligned) type{std::forward<V_args>(p_args) ...};
			advance();
			return ret;
		}
	};
	*/

} // ns