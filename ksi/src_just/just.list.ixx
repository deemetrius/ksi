module;

#include "../src/pre.h"

export module just.list;

import <type_traits>;
export import just.aux;

export namespace just {

	template <typename T_node, bool C_reverse = false>
	struct node_list_iterator {
		using t_node_pointer = T_node *;
		using target_pointer = T_node::target_pointer;

		// data
		t_node_pointer	m_current, m_next;

		node_list_iterator(t_node_pointer p_node) requires(C_reverse) : m_current{p_node}, m_next{p_node->m_prev} {}
		node_list_iterator(t_node_pointer p_node) requires(!C_reverse) : m_current{p_node}, m_next{p_node->m_next} {}

		node_list_iterator & operator ++ () {
			m_current = m_next;
			if constexpr( C_reverse ) { m_next = m_current->m_prev; }
			else { m_next = m_current->m_next; }
			return *this;
		}

		bool operator == (const node_list_iterator & p_other) const { return m_current == p_other.m_current; }
		bool operator != (const node_list_iterator & p_other) const { return m_current != p_other.m_current; }

		//t_node_pointer operator * () { return m_current; }
		target_pointer operator * () { return m_current->node_target(); }
	};

	template <typename T_target>
	struct node_list {
		using target = T_target;
		using target_pointer = target *;
		using node_pointer = node_list *;
		using t_node_iterator = node_list_iterator<node_list>;
		using t_node_range = range_for<t_node_iterator>;
		using t_node_iterator_reverse = node_list_iterator<node_list, true>;
		using t_node_range_reverse = range_for<t_node_iterator_reverse>;
		
		// data
		node_pointer	m_next = this;
		node_pointer	m_prev = this;

		inline target_pointer node_target() {
			return static_cast<target_pointer>(this);
		}

		inline bool node_empty() const {
			return this->m_next == this;
		}
		
		void node_connect_with(node_pointer p_node) {
			p_node->m_prev = this;
			this->m_next = p_node;
		}

		void node_attach(node_pointer p_node) {
			p_node->node_connect_with(this->m_next);
			this->node_connect_with(p_node);
		}
		
		void node_detach() {
			this->m_prev->node_connect_with(this->m_next);
			this->node_reset();
		}
		
		void node_reset() {
			this->m_next = this;
			this->m_prev = this;
		}

		template <typename T_fn>
		void node_apply_to_others(T_fn && p_fn) {
			node_pointer v_current = this->m_next, v_next;
			while( v_current != this ) {
				v_next = v_current->m_next;
				p_fn(v_current);
				v_current = v_next;
			}
		}

		t_node_range node_range() { return {this->m_next, this}; }
		t_node_range_reverse node_range_reverse() { return {this->m_prev, this}; }
	};

	template <typename T_target, template <typename T1> typename T_closer = closers::simple_delete>
	struct list {
		using type = T_target;
		using t_node = node_list<T_target>;
		using t_node_pointer = t_node *;
		using t_closer = T_closer<T_target *>;
		using iterator = t_node::t_node_iterator;
		using t_range_reverse = t_node::t_node_range_reverse;

		static constexpr bool s_need_close = ! std::is_same_v<t_closer, closers::simple_none<T_target *> >;
		
		// data
		t_node	m_zero;
		
		list() = default;
		~list() requires(s_need_close) {
			clear();
		}

		// no no
		list(const list &) = delete;
		list(list &&) = delete;
		list & operator = (const list &) = delete;
		list & operator = (list &&) = delete;

		void clear() {
			if constexpr( s_need_close ) {
				m_zero.node_apply_to_others([](t_node_pointer p_node){
					t_closer::close(p_node->node_target() );
				});
			}
			m_zero.node_reset();
		}

		iterator begin() { return m_zero.m_next; }
		iterator end() { return &m_zero; }
		t_range_reverse range_reverse() { return {m_zero.m_prev, &m_zero}; }

		void append(t_node_pointer p_node) {
			m_zero.m_prev->node_connect_with(p_node);
			p_node->node_connect_with(&m_zero);
		}

		void prepend(t_node_pointer p_node) {
			p_node->node_connect_with(m_zero.m_next);
			m_zero.node_connect_with(p_node);
		}

		void splice(list & p_other) {
			if( p_other.m_zero.node_empty() ) return;
			m_zero.m_prev->node_connect_with(p_other.m_zero.m_next);
			p_other.m_zero.m_prev->node_connect_with(&m_zero);
			p_other.m_zero.node_reset();
		}
	};

} // ns