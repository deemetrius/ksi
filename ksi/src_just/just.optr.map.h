#pragma once

// o_map

template <typename Key, typename Value, typename Less>
struct o_map {
	using reference = Value &;
	using t_optr = optr<Value>;
	using t_items = std::map<Key, t_optr, Less>;
	using iterator = t_items::iterator;
	using t_try_emplace = decltype( std::declval<t_items>().try_emplace( std::declval<Key>() ) );

	// data
	junction::pointer
		mp_owner;
	t_items
		m_items;

	//

	o_map(const o_map &) = delete;
	o_map(o_map &&) = default;

	o_map & operator = (const o_map &) = delete;
	o_map & operator = (o_map &&) = delete;

	//

	o_map(junction::pointer p_owner) : mp_owner{p_owner} {}

	template <typename T_key, typename ... Args>
	t_try_emplace try_emplace(T_key && p_key, Args && ... p_args) {
		return m_items.try_emplace(std::forward<T_key>(p_key), mp_owner, std::forward<Args>(p_args) ...);
	}
};

// ot_map

template <typename Key, typename Value, typename Less>
struct ot_map : public with_point< ot_map<Key, Value, Less> >, public o_map<Key, Value, Less> 
{
	using t_base = o_map<Key, Value, Less>;
	using t_base::t_optr;
	using t_base::t_items;
	using value_type = t_items::value_type;

	ot_map() : t_base{this->m_point.get()} {}

	void unset_elements() {
		for( value_type & v_it : std::ranges::reverse_view{this->m_items} ) {
			//Less::unset(v_it.first);
			v_it.second->unset();
		}
	}
};
