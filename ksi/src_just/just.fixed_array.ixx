module;

#include "../src/pre.h"

export module just.array;

export import just.iter;

export namespace just {

	template <typename T, direction C_direction = direction::forward>
	struct forward_iterator_array {
		using self = forward_iterator_array;
		
		using type = T;
		using pointer = type *;
		using value_type = type &;

		static constexpr direction s_direction = C_direction;

		// data
		pointer
			m_current,
			m_end;

		constexpr value_type operator * () { return *m_current; }

		constexpr self & operator ++ () {
			if constexpr( s_direction == direction::forward ) { ++m_current; }
			else { --m_current; }
			return *this;
		}

		constexpr bool operator == (const self & p_other) const { return m_current == p_other.m_current; }
		constexpr bool operator != (const self & p_other) const { return m_current != p_other.m_current; }

		constexpr bool operator == (sentinel_type) const { return m_current == m_end; }
		constexpr bool operator != (sentinel_type) const { return m_current != m_end; }

		constexpr forward_iterator_array begin() const { return *this; }
		constexpr sentinel_type end() const { return {}; }
	};

	template <typename T, t_size C_size>
	struct array {
		using type = T;
		using pointer = type *;
		using reference = type &;
		using iterator = forward_iterator_array<type>;
		using reverse_iterator = forward_iterator_array<type, direction::reverse>;

		using const_type = const T;
		using const_pointer = const_type *;
		using const_reference = const_type &;
		using const_iterator = forward_iterator_array<const_type>;
		using const_reverse_iterator = forward_iterator_array<const_type, direction::reverse>;

		static constexpr t_size
			s_size = C_size,
			s_last = s_size -1
		;

		// data
		type
			m_items[s_size];

		constexpr sentinel_type end() const { return {}; }

		// non-const

		constexpr iterator begin() { return {m_items, after_last()}; }
		constexpr reverse_iterator reverse() { pointer v = before_first(); return {v + s_size, v}; }

		constexpr pointer before_first() { return m_items -1; }
		constexpr pointer after_last() { return m_items + s_size; }

		constexpr reference first() { return *m_items; }
		constexpr reference last() { return m_items[s_last]; }

		constexpr reference operator [] (t_index p) { return m_items[p]; }

		// const

		constexpr const_iterator begin() const { return {m_items, after_last()}; }
		constexpr const_reverse_iterator reverse() const { const_pointer v = before_first(); return {v + s_size, v}; }

		constexpr const_pointer before_first() const { return m_items -1; }
		constexpr const_pointer after_last() const { return m_items + s_size; }

		constexpr const_reference first() const { return *m_items; }
		constexpr const_reference last() const { return m_items[s_last]; }
		
		constexpr const_reference operator [] (t_index p) const { return m_items[p]; }
	};

} // ns