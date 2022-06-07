module;

#include "../src/pre.h"

export module just.common;

import <cstdint>;
import <cstddef>;
import <concepts>;

export namespace just {
	
	using t_byte_under = unsigned char;
	enum t_byte : t_byte_under {};
	
	using t_int = std::intptr_t;
	using t_uint = std::uintptr_t;
	
	using t_int_max = std::intmax_t;
	using t_uint_max = std::uintmax_t;
	
	using t_diff = std::ptrdiff_t;
	using t_size = std::size_t;
	
	using t_plain_text = const char *;
	using t_plain_text_wide = const wchar_t *;
	
	//

	template <typename T, typename ... U>
	concept c_any_of = ( std::same_as<T, U> || ... );

	template <typename T, typename T_left>
	concept c_inequal_comparable_with_left = requires(const T & p_target, const T_left & p_left) {
		static_cast<bool>(p_left != p_target);
	};

	template <typename T, typename T_left>
	concept c_equal_comparable_with_left = requires(const T & p_target, const T_left & p_left) {
		static_cast<bool>(p_left == p_target);
	};

	//

	template <typename T_iterator, typename T_sentinel = T_iterator>
	struct range_for {
		using iterator = T_iterator;
		using sentinel = T_sentinel;

		// data
		iterator	m_begin;
		sentinel	m_end;

		iterator begin() const { return m_begin; }
		sentinel end() const { return m_end; }
	};

	template <typename T_iterator>
	struct reverse_iterator {
		using iterator = T_iterator;

		// data
		iterator	m_it;

		reverse_iterator(iterator p_it) : m_it{p_it} {}

		decltype(*m_it) operator * () { return *m_it; }
		reverse_iterator & operator ++ () { --m_it; return *this; }

		bool operator != (const reverse_iterator & p_other) const {
			return m_it != p_other.m_it;
		}
		template <c_inequal_comparable_with_left<iterator> T_other_iterator>
		constexpr bool operator != (const T_other_iterator & p_other) const {
			return m_it != p_other;
		}

		bool operator == (const reverse_iterator & p_other) const {
			return m_it == p_other.m_it;
		}
		template <c_equal_comparable_with_left<iterator> T_other_iterator>
		constexpr bool operator == (const T_other_iterator & p_other) const {
			return m_it == p_other;
		}
	};

} // ns

export inline constexpr just::t_byte operator "" _jb (unsigned long long int value) {
	return static_cast<just::t_byte>(value);
}