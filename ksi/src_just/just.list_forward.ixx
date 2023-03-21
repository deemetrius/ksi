module;

#include "../src/pre.h"

export module just.list_forward;

export import just.aux;
export import just.iter;
import <type_traits>;

export namespace just {

	template <typename T_node>
	struct list_forward_iterator_forward {
		using self = list_forward_iterator_forward;
		using t_node_pointer = T_node *;
		using value_type = T_node::target_pointer;

		static t_node_pointer next(t_node_pointer p_node) { return (p_node == nullptr) ? nullptr : p_node->m_next; }

		// data
		t_node_pointer
			m_current,
			m_next;

		list_forward_iterator_forward(t_node_pointer p_node) : m_current{p_node}, m_next{next(p_node)} {}

		self & operator ++ () {
			m_current = m_next;
			m_next = next(m_current);
			return *this;
		}

		bool operator == (const self & p_other) const { return m_current == p_other.m_current; }
		bool operator != (const self & p_other) const { return m_current != p_other.m_current; }

		bool operator == (sentinel_type) const { return m_current == nullptr; }
		bool operator != (sentinel_type) const { return m_current != nullptr; }

		value_type operator * () { return m_current->forward_target(); }
	};

	template <typename T_target>
	struct node_forward {
		using target = T_target;
		using target_pointer = target *;

		using forward_pointer = node_forward *;

		using t_forward_iterator = list_forward_iterator_forward<node_forward>;
		using t_forward_range = range_for<t_forward_iterator, sentinel_type>;

		// data
		forward_pointer
			m_next = nullptr;

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
			this->m_next = nullptr;
		}

		t_forward_range forward_range() { return {this->m_next, {} }; }
	};

	template <typename T_target, template <typename T1> typename T_closer = closers::simple_delete>
	struct list_forward {
		using type = T_target;
		using pointer = type *;
		using t_closer = T_closer<pointer>;

		using t_node = node_forward<type>;
		using t_node_pointer = t_node *;

		using iterator = t_node::t_forward_iterator;

		static constexpr bool s_need_close = ! std::is_same_v<t_closer, closers::simple_none<T_target *> >;

		// data
		t_node
			m_zero;

		list_forward() = default;
		~list_forward() requires(s_need_close) { clear(); }

		// no no
		list_forward(const list_forward &) = delete;
		list_forward(list_forward &&) = delete;
		list_forward & operator = (const list_forward &) = delete;
		list_forward & operator = (list_forward &&) = delete;

		void clear() {
			if constexpr( s_need_close ) {
				for( pointer it : *this ) { t_closer::close(it); }
			}
			m_zero.forward_reset();
		}

		void prepend(t_node_pointer p_node) {
			m_zero.forward_attach(p_node);
		}

		iterator begin() { return {m_zero.m_next}; }
		sentinel_type end() { return {}; }
	};

} // ns