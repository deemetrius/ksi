module;

#include "../src/pre.h"

export module just.std;

import <type_traits>;
import <utility>;
import <cstddef>;
import <cstdint>;

export namespace just {

	struct empty_type {};

	template <typename T>
	struct in_place_type {
		using type = T;
	};

	using t_byte = unsigned char;

	using t_integer = std::intptr_t;
	using t_index = std::ptrdiff_t;
	using t_size = std::size_t;

	constexpr bool g_index_is_integer = std::is_same_v<t_index, t_integer>;

	using t_int_max = std::intmax_t;
	using t_int_max_u = std::uintmax_t;

	using t_const_text = const char *;
	using t_const_text_wide = const wchar_t *;
	using t_text = char *;
	using t_text_wide = wchar_t *;

	template <typename T, typename ... V>
	concept c_any_of = ( std::is_same_v<T, V> || ... );

	template <typename T, typename ... V>
	concept c_not_those = ( !std::is_same_v<T, V> && ... );

	template <typename T_base, typename ... V_derived>
	concept c_base_of = ( std::is_base_of_v<T_base, V_derived> && ... );

	template <typename T, typename T_value, typename ... V_args>
	constexpr bool is_one_of(T p_value, T_value p_first, V_args ... p_args) {
		if constexpr( sizeof...(V_args) == 0 ) { return p_value == p_first; }
		else { return p_value == p_first || is_one_of(p_value, p_args ...); }
	}

	struct none {};

	namespace literal_byte {

		constexpr t_byte operator "" _jb (t_int_max_u p_value) {
			return static_cast<t_byte>(p_value);
		}

	} // ns

} // ns