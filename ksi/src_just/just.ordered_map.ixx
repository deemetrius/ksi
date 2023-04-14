module;

#include "../src/pre.h"

export module just.ordered_map;

export import just.std;
export import <map>;
export import <vector>;
export import <memory>;

export namespace just {

	template <typename T_key, typename T_value, typename T_less = std::ranges::less>
	struct ordered_map {

		using t_key_const_pointer = const T_key *;
		using t_key_cref = const T_key &;
		using t_value_reference = T_value &;

		struct value_type {
			// data
			t_index
				m_index;
			T_value
				m_value;

			template <typename ... T_args>
			value_type(t_index p_index, T_args && ... p_args) :
				m_index{p_index},
				m_value{std::forward<T_args>(p_args) ...}
			{}
		};

		using t_map = std::map<T_key, value_type, T_less>;
		using t_map_iterator = t_map::iterator;
		using t_vec = std::vector<t_map_iterator>;
		using t_vec_iterator = t_vec::iterator;

		using t_map_value_type = t_map::value_type;
		using t_emplace_result = std::pair<t_map_iterator, bool>;

		struct iterator {
			// data
			t_vec_iterator
				m_iter;

			t_map_value_type & operator * () { return **m_iter; }
			iterator & operator ++ () { ++m_iter; return *this; }

			bool operator == (const iterator & m_other) const { return m_iter == m_other.m_iter; }
			bool operator != (const iterator & m_other) const { return m_iter != m_other.m_iter; }
		};

		// data
		t_map
			m_map;
		t_vec
			m_vec;

		iterator begin() { return {m_vec.begin()}; }
		iterator end() { return {m_vec.end()}; }

		template <typename ... T_args>
		t_emplace_result try_emplace(t_key_cref p_key, T_args && ... p_args) {
			t_index v_index = std::ssize(m_vec);
			t_emplace_result ret = m_map.try_emplace(p_key, v_index, std::forward<T_args>(p_args) ...);
			if( ret.second ) { m_vec.emplace_back(ret.first); }
			return ret;
		}

		t_map_iterator find(t_key_cref p_key) {
			return m_map.find(p_key);
		}

		t_map_value_type & operator [] (t_index p_index) {
			return *m_vec[p_index];
		}

	}; // ordered_map

} // ns