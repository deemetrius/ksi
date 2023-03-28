module;

#include "../src/pre.h"

export module just.hive;

export import just.std;
export import <vector>;
export import <map>;

export namespace just {

	template <typename Key, typename Value, typename Less>
	struct hive {
		using t_vec = std::vector<Value>;
		using t_vec_pointer = t_vec *;
		using t_map = std::map<Key, t_index, Less>;
		using t_map_iterator = t_map::iterator;
		using value_pointer = Value *;
		using key_const_pointer = const Key *;
		using t_pair = std::pair<typename t_map::iterator, bool>;

		struct value_type {
			// data
			key_const_pointer
				m_key;
			t_index
				m_index;
			value_pointer
				m_value;
		};

		struct iterator {
			// data
			t_map::iterator
				m_map_it;
			t_vec_pointer
				m_vec;
			bool
				m_added = false;

			value_type operator * () const {
				return {&m_map_it->first, m_map_it->second, &(*m_vec)[m_map_it->second]};
			}

			iterator & operator ++ () { ++m_map_it; return *this; }

			bool operator == (const iterator & p_other) const { return m_map_it == p_other.m_map_it; }
			bool operator != (const iterator & p_other) const { return m_map_it != p_other.m_map_it; }
		};

		// data
		t_vec
			m_vec;
		t_map
			m_map;

		iterator begin() { return {m_map.begin(), &m_vec}; }
		iterator end() { return {m_map.end(), &m_vec}; }

		template <typename ... Args>
		iterator try_emplace(const Key & p_key, Args && ... p_args) {
			t_map_iterator v_it = m_map.find(p_key);
			if( v_it == m_map.end() ) {
				t_index v_id = std::ssize(m_vec);
				t_pair v_pair = m_map.try_emplace(p_key, v_id);
				m_vec.emplace_back(std::forward<Args>(p_args) ...);
				return {v_pair.first, &m_vec, v_pair.second};
			}
			return {v_it, &m_vec, false};
		}

		iterator find(const Key & p_key) {
			return {m_map.find(p_key), &m_vec};
		}
	};

} // ns