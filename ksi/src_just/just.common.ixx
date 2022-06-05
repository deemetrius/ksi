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
	
	template <typename T, typename ... U>
	concept c_any_of = ( std::same_as<T, U> || ... );

} // ns

export inline constexpr just::t_byte operator "" _jb (unsigned long long int value) {
	return static_cast<just::t_byte>(value);
}