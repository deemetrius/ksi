module;

#include "../src/pre.h"

export module just.ref;

import <concepts>;
export import just.aux;

export namespace just {
	
	template <typename T, typename T_traits>
	struct ref : T_traits::template t_inner<T>::t_ref_base {
		using t_traits = typename T_traits::template t_inner<T>;
		using t_base = t_traits::t_ref_base;
		using pointer = t_traits::pointer;
		
		friend void swap(ref & p1, ref & p2) { std::ranges::swap(p1.m_handle, p2.m_handle); }
		
		ref(pointer p_handle) : t_base{p_handle} {}
		ref & operator = (pointer p_handle) {
			t_traits::close(this->m_handle);
			this->m_handle = p_handle;
			return *this;
		}
		
		ref() requires( t_traits::s_allow_default ) = default;
		~ref() { t_traits::close(this->m_handle); }
		
		// copy
		ref(const ref & p_other) requires( t_traits::s_allow_copy ) {
			t_traits::accept_init(this->m_handle, p_other.m_handle);
		}
		ref & operator = (const ref & p_other) {
			t_traits::accept(this->m_handle, p_other.m_handle);
			return *this;
		}
		
		// move
		ref(ref && p_other) requires( t_traits::s_allow_move ) {
			std::ranges::swap(this->m_handle, p_other.m_handle);
		}
		ref & operator = (ref && p_other) {
			std::ranges::swap(this->m_handle, p_other.m_handle);
			return *this;
		}
		
		operator bool () const { return this->m_handle; }
		bool operator ! () const { return !this->m_handle; }
		pointer operator -> () const { return this->m_handle; }
		T & operator * () const { return *this->m_handle; }
	};
	
	template <bool C_check_null, template <typename T1> typename T_closer>
	struct ref_traits_count {
		template <typename T>
		requires ( C_check_null == false || T_closer<T>::s_can_accept_null )
		struct t_inner :
			public T_closer<T>
		{
			using t_ref_base = bases::with_handle<T>;
			using pointer = T *;
			using t_closer = T_closer<T>;
			
			static constexpr bool s_allow_default = C_check_null;
			static constexpr bool s_allow_move = C_check_null;
			static constexpr bool s_allow_copy = true;
			
			static void accept_init(pointer & p_to, pointer p_from) {
				if constexpr ( C_check_null ) { if( p_from ) p_from->refs_inc(); }
				else { p_from->refs_inc(); }
				p_to = p_from;
			}
			
			static void accept(pointer & p_to, pointer p_from) {
				if constexpr ( C_check_null ) { if( p_from ) p_from->refs_inc(); }
				else { p_from->refs_inc(); }
				t_closer::close(p_to);
				p_to = p_from;
			}
		};
	};
	
} // ns