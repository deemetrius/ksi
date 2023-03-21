module;

#include "../src/pre.h"

export module just.iter;

export import just.std;

export namespace just {

	enum class direction : t_byte { forward, reverse };

	struct sentinel_type {};

	template <typename T_iterator, typename T_sentinel = T_iterator>
	struct range_for {
		using iterator = T_iterator;
		using sentinel = T_sentinel;

		// data
		iterator
			m_begin;
		sentinel
			m_end;

		constexpr iterator begin() const { return m_begin; }
		constexpr sentinel end() const { return m_end; }
	};

	template <typename T, typename T_left>
	concept c_inequal_comparable_with_left = requires(const T_left & p_left, const T & p_target) {
		static_cast<bool>(p_left != p_target);
	};

	template <typename T, typename T_left>
	concept c_equal_comparable_with_left = requires(const T_left & p_left, const T & p_target) {
		static_cast<bool>(p_left == p_target);
	};

	template <typename T_iterator>
	struct reverse_iterator_forward {
		using iterator = T_iterator;

		// data
		iterator
			m_it;

		constexpr decltype(auto) operator * () { return *m_it; }
		constexpr reverse_iterator_forward & operator ++ () { --m_it; return *this; }

		constexpr bool operator == (const reverse_iterator_forward & p_other) const {
			return m_it == p_other.m_it;
		}
		template <c_equal_comparable_with_left<iterator> T_other_iterator>
		constexpr bool operator == (const T_other_iterator & p_other) const {
			return m_it == p_other;
		}

		constexpr bool operator != (const reverse_iterator_forward & p_other) const {
			return m_it != p_other.m_it;
		}
		template <c_inequal_comparable_with_left<iterator> T_other_iterator>
		constexpr bool operator != (const T_other_iterator & p_other) const {
			return m_it != p_other;
		}
	};

} // ns
