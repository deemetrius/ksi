module;

#include "../src/pre.h"

export module just.hive;

import <utility>;
import <concepts>;
export import <vector>;
export import <map>;
export import <functional>;

export namespace just {
	
	template <typename T_key, typename T_value, typename T_compare = std::ranges::less>
	struct hive {
		using t_key = T_key;
		using t_key_const_pointer = const t_key *;
		//
		using t_value = T_value;
		using t_value_pointer = t_value *;
		//
		using t_vector = std::vector<t_value>;
		using t_vector_pointer = t_vector *;
		using t_position = t_vector::size_type;
		//
		using t_map = std::map<t_key, t_position, T_compare>;
		using t_map_iterator = t_map::iterator;
		using t_map_insert_result = std::pair<t_map_iterator, bool>;
		//
		using t_find_position = std::pair<bool, t_position>;
		
		struct t_info {
			// data
			t_position			m_index;
			t_key_const_pointer	m_key;
			t_value_pointer		m_value;
		};
		
		struct t_find_result : public t_info {
			// data
			bool				m_added = false;
		};
		
		struct iterator {
			// data
			t_map_iterator		m_map_iterator;
			t_vector_pointer	m_vector;
			
			iterator & operator ++ () {
				++m_map_iterator;
				return *this;
			}
			
			iterator & operator -- () {
				--m_map_iterator;
				return *this;
			}
			
			bool operator == (const iterator & p_it) const {
				return m_map_iterator == p_it.m_map_iterator;
			}
			
			bool operator != (const iterator & p_it) const {
				return m_map_iterator != p_it.m_map_iterator;
			}
			
			t_info operator * () const {
				t_position v_index = (*m_map_iterator).second;
				return {v_index, &(*m_map_iterator).first, m_vector->data() + v_index};
			}
		};
		
		// data
		t_map		m_map;
		t_vector	m_vector;
		
		t_find_position find_position(const t_key & p_key) {
			t_map_iterator res = m_map.find(p_key);
			return res == m_map.end() ? t_find_position{false, 0} : t_find_position{true, (*res).second};
		}
		
		t_find_result find(const t_key & p_key) {
			t_map_iterator res = m_map.find(p_key);
			return res == m_map.end() ? t_find_result{} :
				t_find_result{(*res).second, &(*res).first, m_vector.data() + (*res).second, true}
			;
		}
		
		template <typename ... T_args>
		t_find_result maybe_emplace(const t_key & p_key, T_args && ... p_args) {
			t_position v_position = m_vector.size();
			t_map_insert_result res = m_map.insert({p_key, v_position});
			if( res.second ) {
				m_vector.emplace_back(std::forward<T_args>(p_args) ...);
			} else {
				v_position = (*res.first).second;
			}
			return {v_position, &(*res.first).first, m_vector.data() + v_position, res.second};
		}
		
		iterator begin() { return {m_map.begin(), &m_vector}; }
		iterator end() { return {m_map.end(), &m_vector}; }

		friend void swap(hive & p_1, hive & p_2) {
			std::ranges::swap(p_1.m_map, p_2.m_map);
			std::ranges::swap(p_1.m_vector, p_2.m_vector);
		}
	};
	
} // ns