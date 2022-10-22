module;

#include "../src/pre.h"

export module just.aux;

import <concepts>;
export import just.common;

export namespace just {

	namespace closers {

		template <typename T>
		struct simple_none {
			using type = T;

			static constexpr bool s_can_accept_null = true;

			static void close(type p_handle) {}
		};

		template <typename T>
		struct simple_destructor {
			using type = T;

			static constexpr bool s_can_accept_null = false;

			static void close(type & p_value) { p_value.~type(); }
		};

		template <typename T>
		requires ( std::is_pointer_v<T> )
		struct simple_delete {
			using type = T;

			static constexpr bool s_can_accept_null = true;

			static void close(type p_value) { delete p_value; }
		};

		template <typename T>
		requires ( std::is_pointer_v<T> )
		struct simple_delete_array {
			using type = T;

			static constexpr bool s_can_accept_null = true;

			static void close(type p_value) { delete [] p_value; }
		};

		template <bool C_can_accept_null = false>
		struct compound_call_deleter {
			template <typename T>
			requires ( std::is_pointer_v<T> )
			struct t_closer {
				using type = T;

				static constexpr bool s_can_accept_null = C_can_accept_null;

				static void close(type p_handle) {
					if constexpr( C_can_accept_null ) {
						if( p_handle ) p_handle->m_deleter(p_handle);
					} else {
						p_handle->m_deleter(p_handle);
					}
				}
			};
		};

		template <
			typename T_cast,
			template <typename T1> typename T_closer = simple_delete
		>
		struct compound_cast {
			using t_target = T_cast;
			using t_target_closer = T_closer<t_target>;

			template <typename T>
			struct t_closer {
				using type = T;

				static constexpr bool s_can_accept_null = t_target_closer::s_can_accept_null;

				static void close(type p_handle) {
					t_target_closer::close( static_cast<t_target>(p_handle) );
				}
			};
		};

		template <
			bool C_check_null,
			template <typename T1> typename T_closer = simple_delete
		>
		struct compound_count {
			template <typename T>
			struct t_closer {
				using type = T;
				using t_target_closer = T_closer<type>;

				static constexpr bool s_can_accept_null = C_check_null;

				static void close(type p_handle) {
					if constexpr ( C_check_null ) {
						if( p_handle && p_handle->refs_dec() ) t_target_closer::close(p_handle);
					} else {
						if( p_handle->refs_dec() ) t_target_closer::close(p_handle);
					}
				}
			};
		};

		template <bool C_check_null>
		struct compound_count_call_deleter {
			template <typename T>
			requires ( std::is_pointer_v<T> )
			struct t_closer {
				using type = T;

				static constexpr bool s_can_accept_null = C_check_null;

				static void close(type p_handle) {
					if constexpr ( C_check_null ) {
						if( p_handle ) {
							if( auto v_deleter = p_handle->refs_dec() ) v_deleter(p_handle);
						}
					} else {
						if( auto v_deleter = p_handle->refs_dec() ) v_deleter(p_handle);
					}
				}
			};
		};

	} // ns

	namespace bases {

		template <typename T>
		struct with_handle {
			using pointer = T *;

			// data
			pointer		m_handle = nullptr;

			//friend auto operator <=> (const with_handle &, const with_handle &) = default;
		};

		template <typename T>
		struct with_handle_mutable {
			using pointer = T *;

			// data
			mutable pointer		m_handle = nullptr;
		};

		template <typename T, template <typename T1> typename T_closer = closers::simple_delete>
		struct with_deleter {
			using t_closer = T_closer<T>;
			using t_deleter = decltype(&t_closer::close);

			t_deleter m_deleter = t_closer::close;
		};

		struct with_ref_count {
			using t_refs = t_int_ptr;

			// data
			t_refs	m_refs = 1;

			void refs_inc() { ++m_refs; }
			bool refs_dec() { return --m_refs < 1; }
		};

	} // ns

} // ns