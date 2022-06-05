module;

#include "../src/pre.h"

export module just.list;

import <type_traits>;

export namespace just {
	
	template <typename T_target, typename T_node = void>
	struct node_forward {
		using target = T_target;
		using target_pointer = target *;
		static constexpr bool s_only_forward = std::is_same_v<T_node, void>;
		using forward_pointer = std::conditional_t<s_only_forward, node_forward *, T_node *>;
		
		// data
		forward_pointer 	m_next = static_cast<forward_pointer>(this);
		
		inline target_pointer node_get_target() {
			return static_cast<target_pointer>(this);
		}
		
		inline bool node_empty() const {
			return m_next == this;
		}
		
		template <typename T_fn>
		void apply_to_others(T_fn && p_fn) {
			forward_pointer v_current = this->m_next, v_next;
			while( v_current != this ) {
				v_next = v_current->m_next;
				p_fn(v_current, this->m_next);
				v_current = v_next;
			}
		}
		
		void forward_attach(forward_pointer p_node) requires (s_only_forward) {
			p_node->m_next = this->m_next;
			this->m_next = p_node;
		}
		
		forward_pointer forward_detach_next() requires (s_only_forward) {
			forward_pointer ret = this->m_next;
			this->m_next = ret->m_next;
			return ret;
		}
		
		void forward_reset() requires (s_only_forward) {
			this->next_ = static_cast<forward_pointer>(this);
		}
	};
	
	template <typename T_target>
	struct node_list :
		public node_forward< T_target, node_list<T_target> >
	{
		using target = T_target;
		using target_pointer = target *;
		using node_pointer = node_list *;
		
		// data
		node_pointer	m_prev = this;
		
		void node_attach(node_pointer p_node) {
			p_node->m_prev = this;
			p_node->m_next = this->m_next;
			this->m_next->m_prev = p_node;
			this->m_next = p_node;
		}
		
		void node_detach() {
			this->m_next->m_prev = this->m_prev;
			this->m_prev->m_next = this->m_next;
		}
		
		void node_reset() {
			this->m_next = this;
			this->m_prev = this;
		}
	};
	
	template <typename T_target>
	struct list_forward {
		using t_node = node_forward<T_target>;
		
		// data
		t_node	m_zero;
		
		~list_forward() {
			m_zero.apply_to_others([](t_node::forward_pointer p_node, t_node::forward_pointer p_first){
				delete p_node->node_get_target();
			});
		}
	};
	
	template <typename T_target>
	struct list {
		using t_node = node_list<T_target>;
		
		// data
		t_node	m_zero;
		
		~list() {
			m_zero.apply_to_others([](t_node::forward_pointer p_node, t_node::forward_pointer p_first){
				delete p_node->node_get_target();
			});
		}
	};
	
} // ns