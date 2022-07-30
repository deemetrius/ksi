module;

#include "../src/pre.h"

export module just.pool;

import <type_traits>;
export import just.list_forward;

export namespace just {

	template <typename T, t_size C_size>
	struct pool {
		using pointer = pool *;
		using type = T;
		using type_pointer = type *;

		static constexpr t_size s_size = C_size;

		static constexpr bool s_need_destroy = std::is_trivially_destructible_v<type>;

		struct bucket :
			public node_forward<bucket>
		{
			using pointer = bucket *;
			using t_node = node_forward<bucket>;
			using t_item = aligned_as<type>;
			using t_item_pointer = t_item *;
			using t_reverse_iterator = reverse_iterator<t_item_pointer>;
			using t_reverse_range = range_for<t_reverse_iterator>;

			// data
			t_item	m_items[s_size];

			t_item_pointer begin() { return m_items; }
			t_item_pointer end() { return m_items + s_size; }

			t_reverse_range reverse_for() { return {m_items + s_size -1, m_items -1}; }
			t_reverse_range reverse_for(t_item_pointer p_next) { return {p_next -1, m_items -1}; }
		};

		static void bucket_add(pointer p_pool) {
			typename bucket::pointer v_bucket = new bucket;
			m_zero.forward_attach(v_bucket);
			m_next = v_bucket->begin();
		}

		static void bucket_destroy(bucket::pointer p_bucket) { delete p_bucket; }

		using t_adder = decltype(&bucket_add);
		using t_destroyer = decltype(&bucket_destroy);

		// data
		bucket::t_node			m_zero;
		bucket::t_item_pointer	m_next = nullptr;
		t_adder					m_adder = &bucket_add;
		t_destroyer				m_destroyer = &bucket_destroy;

		pool() { bucket_add(this); }
		~pool() { clear(); }

		// no no
		pool(const pool &) = delete;
		pool(pool &&) = delete;
		pool & operator = (const pool &) = delete;
		pool & operator = (pool &&) = delete;

		void advance() {
			++m_next;
			if( m_next == m_zero.m_next->end() ) { m_adder(this); }
		}

		bucket::pointer bucket_last() { return m_zero->m_next->node_target(); }

		void clear() {
			typename bucket::pointer v_bucket_last = bucket_last();
			typename bucket::t_node::t_forward_range v_range = v_bucket_last->forward_range();
			if constexpr( s_need_destroy ) {
				for( typename bucket::t_item_pointer v_item : v_bucket_last->reverse_for(m_next) ) {
					static_cast<type_pointer>(v_item)->~type();
				}
				for( typename bucket::forward_pointer v_bucket : v_range ) {
					for( typename bucket::t_item_pointer v_item : v_bucket->reverse_for() ) {
						static_cast<type_pointer>(v_item)->~type();
					}
					m_destroyer(v_bucket);
				}
			} else {
				for( typename bucket::forward_pointer v_bucket : v_range ) {
					m_destroyer(v_bucket);
				}
			}
			v_bucket_last->forward_reset();
			m_next = v_bucket_last->begin();
		}
	};

} // ns