module;

#include "../src/pre.h"

export module just.map;

export import <functional>;
export import <utility>;
export import <map>;
export import just.list;

export namespace just {

	template <typename T_key, typename T_value, typename T_compare = std::ranges::less>
	struct map {
		using t_key = T_key;
		using t_value = T_value;
		using t_compare = T_compare;

		struct t_node :
			public node_list<t_node>
		{
			using pointer = t_node *;
			using t_map = std::map<t_key, t_node, t_compare>;
			using t_key_iterator = t_map::iterator;

			// data
			t_key_iterator	m_key_iterator;
			t_value			m_value;

			template <typename ... T_args>
			t_node(T_args && ... p_args) : m_value{std::forward<T_args>(p_args) ...} {}
		};

		using t_map = t_node::t_map;
		using t_key_iterator = t_node::t_key_iterator;
		using t_add_result = std::pair<t_key_iterator, bool>;
		using t_list = node_list<t_node>;
		using iterator = t_list::t_node_iterator;
		using t_range_reverse = t_list::t_node_range_reverse;
		//using t_base_node_pointer = t_node::node_pointer;

		// data
		t_map	m_map;
		t_list	m_list;

		map() = default;

		// no no
		map(const map &) = delete;
		map(map &&) = delete;
		map & operator = (const map &) = delete;
		map & operator = (map &&) = delete;

		// iteration
		iterator begin() { return {m_list.m_next}; }
		iterator end() { return {&m_list}; }
		t_range_reverse range_reverse() { return m_list.node_range_reverse(); }

		void clear() {
			m_list.node_reset();
			m_map.clear();
		}

		t_index count() const { return std::ssize(m_map); }

		t_node::pointer find(const t_key & p_key) {
			t_key_iterator v_it = m_map.find(p_key);
			return (v_it == m_map.end() ) ? nullptr : &(*v_it).second;
		}

		template <typename ... T_args>
		t_add_result maybe_emplace(const t_key & p_key, T_args && ... p_args) {
			t_add_result ret = m_map.try_emplace(p_key, std::forward<T_args>(p_args) ...);
			if( ret.second ) {
				typename t_node::pointer v_node = &(*ret.first).second;
				v_node->m_key_iterator = ret.first;
				m_list.m_prev->node_attach(v_node);
			}
			return ret;
		}

		template <typename ... T_args>
		t_add_result maybe_emplace(t_key && p_key, T_args && ... p_args) {
			t_add_result ret = m_map.try_emplace(std::move(p_key), std::forward<T_args>(p_args) ...);
			if( ret.second ) {
				typename t_node::pointer v_node = &(*ret.first).second;
				v_node->m_key_iterator = ret.first;
				m_list.m_prev->node_attach(v_node);
			}
			return ret;
		}

		t_key_iterator remove(t_key_iterator p_key_iterator) {
			(*p_key_iterator).second.node_detach();
			return m_map.erase(p_key_iterator);
		}

		t_index remove(const t_key & p_key) {
			t_key_iterator v_key_iterator = m_map.find(p_key);
			if( v_key_iterator == m_map.end() ) { return 0; }
			remove(v_key_iterator);
			return 1;
		}
	};

} // ns