module;

#include "../src/pre.h"

export module just.flags_bits;

import <type_traits>;
import <concepts>;

export namespace just {

	template <typename T_enum>
	concept c_enum = std::is_enum_v<T_enum>;

	template <c_enum T_enum>
	constexpr std::underlying_type_t<T_enum> to_underlying(T_enum p_value) noexcept {
		return static_cast< std::underlying_type_t<T_enum> >(p_value);
	}

	//

	template <c_enum T_enum, template <typename> typename T_traits>
	struct flags_bits {
		using traits = T_traits<T_enum>;
		using type = traits::type;

		template <std::same_as<T_enum> ... T_args>
		static constexpr type merge(T_args ... p_args) {
			return (traits::convert(p_args) | ...);
		}

		// data
		type	m_value = 0;

		constexpr flags_bits() = default;

		template <std::same_as<T_enum> ... T_args>
		constexpr flags_bits(T_args ... p_args) : m_value{merge(p_args ...)} {}

		template <std::same_as<T_enum> ... T_args>
		constexpr flags_bits set(T_args ... p_args) {
			m_value |= merge(p_args ...);
			return *this;
		}

		template <std::same_as<T_enum> ... T_args>
		constexpr flags_bits unset(T_args ... p_args) {
			m_value &= ~ merge(p_args ...);
			return *this;
		}

		template <std::same_as<T_enum> ... T_args>
		constexpr bool has_all(T_args ... p_args) const {
			type v_value = merge(p_args ...);
			return (m_value & v_value) == v_value;
		}

		template <std::same_as<T_enum> ... T_args>
		constexpr bool has_any(T_args ... p_args) const {
			return m_value & merge(p_args ...);
		}

		template <std::same_as<T_enum> ... T_args>
		constexpr bool has_none(T_args ... p_args) const {
			return (m_value & merge(p_args ...) ) == 0;
		}
	};

	//

	template <c_enum T_enum>
	struct traits_flags {
		using type = std::underlying_type_t<T_enum>;

		static inline type convert(T_enum p_value) { return to_underlying(p_value); }
	};

	template <c_enum T_enum>
	struct traits_bits {
		using type = std::underlying_type_t<T_enum>;
		static constexpr type s_one = 1;

		static inline type convert(T_enum p_value) { return s_one << to_underlying(p_value); }
	};

	//

	template <c_enum T_enum>
	using flags = flags_bits<T_enum, traits_flags>;

	template <c_enum T_enum>
	using bits = flags_bits<T_enum, traits_bits>;

} // ns