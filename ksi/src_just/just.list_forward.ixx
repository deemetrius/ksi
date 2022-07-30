module;

#include "../src/pre.h"

export module just.list_forward;

import <type_traits>;
export import just.aux;

export namespace just {

	struct node_forward_sentinel {};

	template <typename T_node>
	struct node_forward_iterator {
		using t_node_pointer = T_node *;
		using target_pointer = T_node::target_pointer;

		static t_node_pointer next(t_node_pointer p_node) { return (p_node == nullptr) ? nullptr : p_node->m_next; }

		// data
		t_node_pointer	m_current, m_next;

		node_forward_iterator(t_node_pointer p_node) : m_current{p_node}, m_next{next(p_node)} {}

		node_forward_iterator & operator ++ () {
			m_current = m_next;
			m_next = next(m_current);
			return *this;
		}

		bool operator == (const node_forward_iterator & p_other) const { return m_current == p_other.m_current; }
		bool operator != (const node_forward_iterator & p_other) const { return m_current != p_other.m_current; }

		bool operator == (node_forward_sentinel) const { return m_current == nullptr; }
		bool operator != (node_forward_sentinel) const { return m_current != nullptr; }

		//t_node_pointer operator * () { return m_current; }
		target_pointer operator * () { return m_current->forward_target(); }
	};

	template <typename T_target>
	struct node_forward {
		using target = T_target;
		using target_pointer = target *;
		using forward_pointer = node_forward *;
		using t_forward_iterator = node_forward_iterator<node_forward>;
		using t_forward_range = range_for<t_forward_iterator, node_forward_sentinel>;

		// data
		forward_pointer		m_next = nullptr;

		inline target_pointer forward_target() {
			return static_cast<target_pointer>(this);
		}

		inline bool forward_empty() const  {
			return m_next == nullptr;
		}

		void forward_attach(forward_pointer p_node) {
			p_node->m_next = this->m_next;
			this->m_next = p_node;
		}

		forward_pointer forward_detach_next() {
			forward_pointer ret = this->m_next;
			this->m_next = ret->m_next;
			return ret;
		}

		void forward_reset() {
			this->next_ = nullptr;
		}

		template <typename T_fn>
		void forward_apply_to_others(T_fn && p_fn) {
			forward_pointer v_current = this->m_next, v_next;
			while( v_current->forward_empty() ) {
				v_next = v_current->m_next;
				p_fn(v_current);
				v_current = v_next;
			}
		}

		t_forward_range forward_range() { return {this->m_next, {} }; }
	};

	template <typename T_target, template <typename T1> typename T_closer = closers::simple_delete>
	struct list_forward {
		using t_node = node_forward<T_target>;
		using t_node_pointer = t_node *;
		using t_closer = T_closer<T_target *>;
		using iterator = t_node::t_forward_iterator;

		static constexpr bool s_need_close = ! std::is_same_v<t_closer, closers::simple_none<T_target *> >;

		// data
		t_node	m_zero;

		list_forward() = default;
		~list_forward() requires(s_need_close) {
			clear();
		}

		// no no
		list_forward(const list_forward &) = delete;
		list_forward(list_forward &&) = delete;
		list_forward & operator = (const list_forward &) = delete;
		list_forward & operator = (list_forward &&) = delete;

		void clear() {
			m_zero.forward_apply_to_others([](t_node::forward_pointer p_node){
				t_closer::close(p_node->forward_target() );
				});
			m_zero.forward_reset();
		}

		void prepend(t_node_pointer p_node) {
			m_zero.forward_attach(p_node);
		}

		iterator begin() { return {m_zero.m_next}; }
		node_forward_sentinel end() { return {}; }
	};

} // ns