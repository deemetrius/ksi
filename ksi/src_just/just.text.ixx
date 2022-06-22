module;

#include "../src/pre.h"

export module just.text;

import <utility>;
export import just.ref;
import <cstring>;
import <cwchar>;
export import <initializer_list>;

export namespace just {
	
	template <typename T_char, t_size N>
	struct fixed_string {
		using type = T_char;
		enum n_size : t_size { s_length = N -1 };
		
		// data
		type m_text[N];
		
		constexpr fixed_string() requires(N == 1) : m_text{0} {}
		
		template <t_size ... C>
		constexpr fixed_string(const type (&p_text)[N], std::index_sequence<C ...>) :
			m_text{p_text[C] ...}
		{}
		
		constexpr fixed_string(const type (&p_text)[N]) :
			fixed_string( p_text, std::make_index_sequence<N>() )
		{}
	};
	
	namespace detail {
		
		template <typename T_char>
		struct impl_text;
		
		template <typename T_char>
		struct impl_text_base {
			using type = T_char;
			using pointer = type *;
			using const_pointer = const type *;
			using t_with = bases::with_deleter<
				const impl_text_base<type> *,
				closers::compound_cast<const impl_text<type> *>::template t_closer
			>;
			using t_deleter = t_with::t_deleter;
			using t_length = t_int_max;
			
			// data
			const_pointer	m_text;
			t_length		m_length;
			
			virtual void refs_inc() const {}
			virtual t_deleter refs_dec() const { return nullptr; }

			virtual t_length size() const { return m_length; }
			virtual bool size(t_length p_length) const { return false; }
			
			constexpr impl_text_base(const_pointer p_text, t_length p_length) :
				m_text(p_text), m_length(p_length)
			{}
		};
		
		template <typename T_char>
		struct impl_text :
			public impl_text_base<T_char>,
			public impl_text_base<T_char>::t_with
		{
			using type = T_char;
			using t_base = impl_text_base<type>;
			using typename t_base::pointer;
			using typename t_base::const_pointer;
			using typename t_base::t_deleter;
			using typename t_base::t_length;
			using t_refs = t_int;
			
			static void close(const_pointer p_handle) { delete [] p_handle; }

			using t_closer = decltype(&close);

			// data
			mutable t_refs		m_refs = 1;
			mutable t_length	m_length_actual;
			t_closer			m_closer = &close;
			
			void refs_inc() const override { ++m_refs; }
			t_deleter refs_dec() const override { return --m_refs < 1 ? this->m_deleter : nullptr; }

			t_length size() const override { return m_length_actual; }
			bool size(t_length p_length) const override { m_length_actual = p_length; return true; }
			
			impl_text(pointer & p_text, t_length p_length) :
				t_base(p_text = new type[p_length +1], p_length),
				m_length_actual(p_length)
			{}
			~impl_text() { m_closer(this->m_text); }
		};
		
		template <typename T> T plain(const T &);
		
		template <fixed_string C>
		struct static_data {
			using type = decltype( plain(*C.m_text) );
			using t_impl_base = impl_text_base<type>;
			
			static constexpr const t_impl_base s_value{C.m_text, C.s_length};
		};
		
	} // ns
	
	namespace text_literals {
		
		template <fixed_string C>
		constexpr auto operator "" _jt () {
			return &detail::static_data<C>::s_value;
		}
		
	} // ns
	
	template <typename T_char>
	struct basic_text {
		using type = T_char;
		using t_impl_base = detail::impl_text_base<type>;
		using t_impl = detail::impl_text<type>;
		using t_ref = ref<const t_impl_base,
			ref_traits_count<false, closers::compound_count_call_deleter<false>::t_closer>
		>;
		using pointer = t_impl_base::pointer;
		using const_pointer = t_impl_base::const_pointer;
		using t_length = t_impl_base::t_length;
		
		static constexpr const type s_empty[1]{};
		
		friend void swap(basic_text & p1, basic_text & p2) { std::ranges::swap(p1.m_ref, p2.m_ref); }
		
		// data
		t_ref m_ref;
		
		basic_text() : basic_text(&detail::static_data<s_empty>::s_value) {}
		basic_text(const t_impl_base * p_impl) : m_ref(p_impl) {}
		basic_text & operator = (const t_impl_base * p_impl) {
			m_ref = p_impl;
			return *this;
		}
		
		basic_text(t_length p_length, pointer & p_text) :
			m_ref( new t_impl(p_text, p_length) )
		{ *p_text = 0; }

		basic_text(const_pointer p_begin, const_pointer p_end, pointer p_text = nullptr, t_length p_length = 0) :
			m_ref( new t_impl(p_text, p_length = p_end - p_begin) )
		{ std::memcpy(p_text, p_begin, p_length); p_text[p_length] = 0; }
		
		//
		operator bool () const { return *m_ref->m_text; }
		bool operator ! () const { return !*m_ref->m_text; }
		const t_impl_base * operator -> () const { return m_ref.m_handle; }
		//operator std::basic_string_view<type> () const { return {m_ref->m_text, m_ref->m_length}; }
	};
	
	using text = basic_text<char>;
	using Text_wide = basic_text<wchar_t>;
	
	template <typename T_char>
	struct text_traits;

	template <>
	struct text_traits<char> {
		static int cmp(t_plain_text p_1, t_plain_text p_2) {
			return std::strcmp(p_1, p_2);
		}
	};

	template <>
	struct text_traits<wchar_t> {
		static int cmp(t_plain_text_wide p_1, t_plain_text_wide p_2) {
			return std::wcscmp(p_1, p_2);
		}
	};

	struct text_less {
		template <typename T_char>
		bool operator () (const basic_text<T_char> & p_1, const basic_text<T_char> & p_2) const {
			return text_traits<T_char>::cmp(p_1->m_text, p_2->m_text) < 0;
		}
	};

	text implode(std::initializer_list<text> p_list, const text & p_separator = text{}) {
		text::t_length v_length = 0;
		for( const text & v_it : p_list ) {
			v_length += v_it->size();
		}
		if( p_separator ) {
			v_length += p_separator->size() * (p_list.size() -1);
			text::pointer v_text;
			text v_ret{v_length, v_text};
			v_text[v_length] = 0;
			bool v_first = true;
			text::t_length v_part_length;
			for( const text & v_it : p_list ) {
				if( v_first ) {
					v_first = false;
				} else {
					v_part_length = p_separator->size();
					std::memcpy(v_text, p_separator->m_text, v_part_length);
					v_text += v_part_length;
				}
				v_part_length = v_it->size();
				std::memcpy(v_text, v_it->m_text, v_part_length);
				v_text += v_part_length;
			}
			return v_ret;
		}
		text::pointer v_text;
		text v_ret{v_length, v_text};
		v_text[v_length] = 0;
		text::t_length v_part_length;
		for( const text & v_it : p_list ) {
			v_part_length = v_it->size();
			std::memcpy(v_text, v_it->m_text, v_part_length);
			v_text += v_part_length;
		}
		return v_ret;
	}

} // ns