#pragma once

// o_vector

template <typename T>
struct o_vector {
	using pointer = T *;
	using t_optr = optr<T>;
	using t_items = just::vector<t_optr>;

	// data
	junction::pointer
		mp_owner;
	t_items
		m_items;

	//

	o_vector(const o_vector &) = delete;
	o_vector(o_vector &&) = default;

	o_vector & operator = (const o_vector &) = delete;
	o_vector & operator = (o_vector &&) = delete;

	//

	o_vector(junction::pointer p_owner) : mp_owner{p_owner} {}

	o_vector(t_size p_size, junction::pointer p_owner) : mp_owner{p_owner} {
		m_items.reserve(p_size);
	}

	template <typename ... Args>
	o_vector(junction::pointer p_owner, t_size p_size, const Args & ... p_args) :
		mp_owner{p_owner},
		m_items{p_size > 0 ? p_size : t_items::t_grow::get_initial_reserve()}
	{
		if( p_size > 0 ) {
			for( size_t i = 0; i < p_size; ++i ) {
				m_items.emplace_back(mp_owner, p_args ...);
			}
		}
	}

	template <typename ... Args>
	pointer emplace_back(Args && ... p_args) {
		t_optr & v = m_items.emplace_back(mp_owner, std::forward<Args>(p_args) ...);
		return v.get();
	}

	void pop_back() { m_items.pop_back(); }

	t_optr & operator [] (t_index p_index) { return m_items[p_index]; }

	t_optr & back() { return m_items.back(); }

	t_index ssize() const { return m_items.size(); }
};

// ot_vector

template <typename T>
struct ot_vector : public is_target< ot_vector<T> >, public o_vector<T> 
{
	using t_base = o_vector<T>;
	using t_optr = t_base::t_optr;

	ot_vector() : t_base{this->m_point.get()} {}

	void unset_elements() {
		for( t_optr & v_it : this->m_items.reverse_range() ) { v_it->unset(); }
	}
};
