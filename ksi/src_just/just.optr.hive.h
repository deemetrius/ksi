#pragma once

// o_hive

template <typename Key, typename Value, typename Less>
struct o_hive {
	using t_optr = optr<Value>;
	using t_vector = std::vector<t_optr>;
	using t_vector_pointer = t_vector *;
	using t_map = std::map<Key, t_index, Less>;
	using map_iterator = t_map::iterator;
	using pointer = Value *;
	using key_const_pointer = const Key *;
	
	struct value_type {
		// data
		key_const_pointer
			m_key;
		t_index
			m_index;
		pointer
			m_value;
	};

	struct iterator {
		// data
		t_map::iterator
			m_map_it;
		t_vector_pointer
			m_vec;
		bool
			m_is_added = false;

		value_type operator * () const {
			return {&m_map_it->first, m_map_it->second, (*m_vec)[m_map_it->second].get()};
		}

		iterator & operator ++ () { ++m_map_it; return *this; }

		bool operator == (const iterator & p_other) const { return m_map_it == p_other.m_map_it; }
		bool operator != (const iterator & p_other) const { return m_map_it != p_other.m_map_it; }
	};

	using t_map_try_emplace = std::pair<typename t_map::iterator, bool>;

	// data
	junction::pointer
		mp_owner;
	t_map
		m_map;
	t_vector
		m_vec;

	// no copy, but move

	o_hive(const o_hive &) = delete;
	o_hive(o_hive &&) = default;

	// no copy assign, no move assign

	o_hive & operator = (const o_hive &) = delete;
	o_hive & operator = (o_hive &&) = delete;

	//

	o_hive(junction::pointer p_owner) : mp_owner{p_owner} {}

	iterator begin() { return {m_map.begin(), &m_vec}; }
	iterator end() { return {m_map.end(), &m_vec}; }

	iterator find(const Key & p_key) {
		return {m_map.find(p_key), &m_vec};
	}

	template <typename T_key, typename ... Args>
	iterator assign(T_key && p_key, Args && ... p_args) {
		map_iterator v_map_it = m_map.find(p_key);
		if( v_map_it == m_map.end() ) {
			t_index v_index = std::ssize(m_vec);
			t_map_try_emplace v_map_try = m_map.try_emplace(std::forward<T_key>(p_key), v_index);
			m_vec.emplace_back(mp_owner, std::forward<Args>(p_args) ...);
			return {v_map_try.first, &m_vec, v_map_try.second};
		}
		t_index v_index = v_map_it.second;
		m_vec.erase( m_vec.begin() + v_index );
		m_vec.emplace( m_vec.cbegin() + v_index, mp_owner, std::forward<Args>(p_args) ... );
		//m_vec[v_index].~t_optr();
		//std::construct_at(&m_vec[v_index], mp_owner, std::forward<Args>(p_args) ... );
		return {v_map_it, &m_vec, false};
	}

	template <typename T_key, typename ... Args>
	iterator try_emplace(T_key && p_key, Args && ... p_args) {
		t_index v_index = std::ssize(m_vec);
		t_map_try_emplace v_map_try = m_map.try_emplace(std::forward<T_key>(p_key), v_index);
		if( v_map_try.second ) {
			m_vec.emplace_back(mp_owner, std::forward<Args>(p_args) ...);
		}
		return {v_map_try.first, &m_vec, v_map_try.second};
	}

	t_index ssize() const {
		return std::ssize(m_vec);
	}

	t_optr & operator [] (t_index p_index) {
		return m_vec[p_index];
	}
};

// ot_hive

template <typename Key, typename Value, typename Less>
struct ot_hive : public is_target< ot_hive<Key, Value, Less> >, public o_hive<Key, Value, Less> 
{
	using t_base = o_hive<Key, Value, Less>;
	using t_base::t_optr;
	using t_base::t_vector;
	using t_base::t_map;
	using t_vector_value_type = t_vector::value_type;
	using t_map_value_type = t_map::value_type;

	ot_hive() : t_base{this->m_point.get()} {}

	void unset_elements() {
		for( t_vector_value_type & v_it : std::ranges::reverse_view{this->m_vec} ) { v_it.second->unset(); }
		//for( t_map_value_type & v_it : std::ranges::reverse_view{this->m_map} ) { Less::unset(v_it.first); }
	}
};
