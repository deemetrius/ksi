module;

#include "../src/pre.h"

export module just.hive;

import <utility>;
import <concepts>;
export import <map>;
export import <functional>;
export import just.array;

export namespace just {

	template <typename T_key, typename T_value, typename T_compare = std::ranges::less>
	struct hive {
		using pointer = hive *;
		//
		using t_key = T_key;
		using t_key_const_pointer = const t_key *;
		//
		using t_value = T_value;
		using t_value_pointer = t_value *;
		//
		using t_vector = array_alias<t_value, just::capacity_step<8, 8> >;
		using t_vector_pointer = t_vector *;
		//
		using t_map = std::map<t_key, t_index, T_compare>;
		using t_map_iterator = t_map::iterator;
		using t_map_insert_result = std::pair<t_map_iterator, bool>;
		//
		using t_keys = std::vector<t_map_iterator>;
		using t_find_position = std::pair<bool, t_index>;

		struct t_info {
			// data
			t_index				m_index = 0;
			t_map_iterator		m_map_iterator;
			//t_key				m_key;
			t_value_pointer		m_value = nullptr;

			const t_key & key() const { return (*m_map_iterator).first; }
		};

		struct t_find_result : public t_info {
			// data
			bool	m_added = false;
		};

		struct key_iterator {
			// data
			t_map_iterator		m_map_iterator;
			t_vector_pointer	m_vector;

			key_iterator & operator ++ () {
				++m_map_iterator;
				return *this;
			}

			key_iterator & operator -- () {
				--m_map_iterator;
				return *this;
			}

			bool operator == (const key_iterator & p_it) const {
				return m_map_iterator == p_it.m_map_iterator;
			}

			bool operator != (const key_iterator & p_it) const {
				return m_map_iterator != p_it.m_map_iterator;
			}

			t_info operator * () const {
				t_index v_index = (*m_map_iterator).second;
				return {v_index, m_map_iterator, m_vector->data() + v_index};
			}
		};

		struct iterator {
			// data
			pointer		m_hive;
			t_index		m_index;

			iterator & operator ++ () {
				++m_index;
				return *this;
			}

			iterator & operator -- () {
				--m_index;
				return *this;
			}

			bool operator == (const iterator & p_it) const {
				return m_index == p_it.m_index;
			}

			bool operator != (const key_iterator & p_it) const {
				return m_index != p_it.m_index;
			}

			t_info operator * () const {
				return {m_index, m_hive->m_keys[m_index], m_hive->m_vector.data() + m_index};
			}
		};

		using t_key_range = range_for<key_iterator>;
		using t_range = range_for<iterator>;

		// data
		t_map		m_map;
		t_vector	m_vector;
		t_keys		m_keys;

		t_find_position find_position(const t_key & p_key) {
			t_map_iterator res = m_map.find(p_key);
			return res == m_map.end() ? t_find_position{false, 0} : t_find_position{true, (*res).second};
		}

		t_find_result find(const t_key & p_key) {
			t_map_iterator v_it = m_map.find(p_key);
			if( v_it == m_map.end() ) { return {}; }
			t_index v_index = (*v_it).second;
			return {v_index, v_it, m_vector.data() + v_index, true};
		}

		template <typename ... T_args>
		t_find_result maybe_emplace(const t_key & p_key, T_args && ... p_args) {
			t_index v_index = m_vector.size();
			t_map_insert_result v_res = m_map.insert({p_key, v_index});
			if( v_res.second ) {
				array_append(m_vector, std::forward<T_args>(p_args) ...);
				m_keys.push_back(v_res.first);
			} else {
				v_index = (*v_res.first).second;
			}
			return {v_index, v_res.first, m_vector.data() + v_index, v_res.second};
		}

		void clear() {
			m_map.clear();
			m_vector.clear();
			m_keys.clear();
		}

		t_index count() { return m_vector.size(); }

		t_key_range key_range() { return { {m_map.begin(), &m_vector}, {m_map.end(), &m_vector} }; }
		iterator begin() { return {this, 0}; }
		iterator end() { return {this, count()}; }
		t_range range() { return {begin(), end()}; }

		friend void swap(hive & p_1, hive & p_2) {
			std::ranges::swap(p_1.m_map, p_2.m_map);
			std::ranges::swap(p_1.m_vector, p_2.m_vector);
			std::ranges::swap(p_1.m_keys, p_2.m_keys);
		}
	};

} // ns