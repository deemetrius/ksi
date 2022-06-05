module;

#include "../src/pre.h"

export module just.aux;

import <concepts>;
export import just.common;

export namespace just {

	namespace closers {

		template <typename T>
		struct simple_delete {
			using const_pointer = const T *;

			static constexpr bool s_can_accept_null = true;

			static void close(const_pointer p_handle) { delete p_handle; }
		};

		template <typename T>
		struct simple_delete_array {
			using const_pointer = const T *;

			static constexpr bool s_can_accept_null = true;

			static void close(const_pointer p_handle) { delete [] p_handle; }
		};

		template <typename T_cast, bool C_const_close = false,
			template <typename T1> typename T_closer = simple_delete
		>
		struct compound_cast {
			using t_target = T_cast;
			using t_target_pointer = t_target *;
			using t_target_const_pointer = const t_target *;
			using t_target_pass = std::conditional_t<C_const_close,
				t_target_const_pointer, t_target_pointer
			>;
			using t_target_closer = T_closer<t_target>;

			template <typename T>
			struct t_closer {
				using type = T;
				using pointer = type *;
				using const_pointer = const type *;
				using t_pass = std::conditional_t<C_const_close, const_pointer, pointer>;

				static constexpr bool s_can_accept_null = t_target_closer::s_can_accept_null;

				static void close(t_pass p_handle) {
					t_target_closer::close( static_cast<t_target_pass>(p_handle) );
				}
			};
		};

		template <
			bool C_check_null,
			bool C_const_close = false,
			template <typename T1> typename T_closer = simple_delete
		>
		struct compound_count {
			template <typename T>
			struct t_closer {
				using type = T;
				using pointer = type *;
				using const_pointer = const type *;
				using t_target_closer = T_closer<type>;
				using t_pass = std::conditional_t<C_const_close, const_pointer, pointer>;

				static constexpr bool s_can_accept_null = C_check_null;

				static void close(t_pass p_handle) {
					if constexpr ( C_check_null ) {
						if( p_handle && p_handle->refs_dec() ) t_target_closer::close(p_handle);
					} else { if( p_handle->refs_dec() ) t_target_closer::close(p_handle); }
				}
			};
		};

		template <bool C_check_null>
		struct compound_count_call_deleter {
			template <typename T>
			struct t_closer {
				using const_pointer = const T *;

				static constexpr bool s_can_accept_null = C_check_null;

				static void close(const_pointer p_handle) {
					if constexpr ( C_check_null ) {
						if( p_handle ) {
							if( auto v_deleter = p_handle->refs_dec() ) v_deleter(p_handle);
						}
					} else { if( auto v_deleter = p_handle->refs_dec() ) v_deleter(p_handle); }
				}
			};
		};

	} // ns

	namespace bases {

		template <typename T>
		struct with_handle {
			using pointer = T *;

			// data
			pointer m_handle = nullptr;

			//friend auto operator <=> (const with_handle &, const with_handle &) = default;
		};

		template <typename T, template <typename T1> typename T_closer = closers::simple_delete>
		struct with_deleter {
			using t_closer = T_closer<T>;
			using t_deleter = decltype(&t_closer::close);

			t_deleter m_deleter = t_closer::close;
		};

		struct with_ref_count {
			using t_refs = t_int;

			// data
			t_refs	m_refs = 1;

			void refs_inc() { ++m_refs; }
			bool refs_dec() { return --m_refs < 1; }
		};

	} // ns

} // ns