module;

#include "../src/pre.h"

export module just.choice;

export import <type_traits>;
export import <utility>;
export import just.common;

export namespace just {

	inline constexpr t_index choice_none = -1;

	template <typename T, typename T_first, typename ... T_rest>
	constexpr t_index index_of(t_index p_from = 0) {
		if constexpr( std::is_same_v<T, T_first> ) { return p_from; }
		//if constexpr ( sizeof...(T_rest) == 0 ) { return choice_none; }
		static_assert(sizeof...(T_rest) > 0, "Given type is absent in variadic pack.");
		return index_of<T, T_rest>(p_from +1);
	}

	namespace detail {
	
		template <t_index C_pos, typename ... T_items>
		requires (C_pos > 0 && C_pos < sizeof...(T_items))
		struct choice_at;

		template <t_index C_pos, typename T_first, typename ... T_rest>
		struct choice_at<C_pos, T_first, T_rest ...> {
			using type = std::conditional_t<C_pos == 0, T_first, typename choice_at<C_pos -1, T_rest ...>::type>;
		};
	
		template <typename T>
		struct choice_traits_non_trivial {
			static void deleter(t_byte_under * p_data) { reinterpret_cast<T *>(p_data)->~T(); }
		};
		struct choice_traits_trivial {
			static void deleter(t_byte_under * p_data) {}
		};
		template <typename T>
		struct choice_traits :
			std::conditional_t<std::is_trivially_destructible_v<T>, choice_traits_trivial, choice_traits_non_trivial<T> >
		{
			static void initer(t_byte_under * p_to, const t_byte_under * p_from) {
				new (p_to) T(*reinterpret_cast<const T *>(p_from));
			}
		};

	} // ns

	template <t_index C_pos, typename ... T_items>
	using choice_at_t = detail::choice_at<C_pos, T_items ...>::type;

	template <typename ... T_items>
	struct choice {
		static constexpr t_size
		c_max_size = std::max<t_size>({sizeof(T_items) ...}),
		c_max_align = std::max<t_size>({alignof(T_items) ...});

		static constexpr decltype(&detail::choice_traits< choice_at_t<0, T_items ...> >::deleter)
		s_deleters[sizeof...(T_items)]{&detail::choice_traits<T_items>::deleter ...};

		static constexpr decltype(&detail::choice_traits< choice_at_t<0, T_items ...> >::initer)
		s_initers[sizeof...(T_items)]{&detail::choice_traits<T_items>::initer ...};

		// data
		alignas(c_max_align) t_byte_under	m_data[c_max_size];
		t_index								m_index = choice_none;

		void clear() {
			if( m_index > 0 ) { s_deleters[m_index](&m_data); m_index = choice_none; }
		}

		template <c_any_of<T_items...> T>
		T & get() { return *static_cast<T *>(&m_data); }

		operator bool () const { return m_index != choice_none; }

		choice() = default;
		~choice() { clear(); }

		// in_place
		template <c_any_of<T_items...> T, typename ... T_args>
		choice(std::in_place_type_t<T>, T_args && ... p_args) {
			new(&m_data) T{std::forward<T_args>(p_args) ...};
			m_index = index_of<T, T_items ...>();
		}

		// T
		template <c_any_of<T_items...> T>
		choice(const T & p_other) { // copy
			forward_from(p_other);
		}
		template <c_any_of<T_items...> T>
		choice(T && p_other) { // move
			forward_from(p_other);
		}
		template <c_any_of<T_items...> T>
		choice & operator = (const T & p_other) { // copy
			clear();
			forward_from(p_other);
			return *this;
		}
		template <c_any_of<T_items...> T>
		choice & operator = (T && p_other) { // move
			clear();
			forward_from(p_other);
			return *this;
		}

		// choice
		choice(const choice & p_other) { // copy
			copy_from(p_other);
		}
		choice(choice && p_other) { // move
			move_from(p_other);
		}
		choice & operator = (const choice & p_other) { // copy
			clear();
			copy_from(p_other);
			return *this;
		}
		choice & operator = (choice && p_other) { // move
			clear();
			move_from(p_other);
			return *this;
		}

	private:
		void copy_from(const choice & p_other) {
			s_initers[p_other.m_index](&m_data, &p_other.m_data);
			m_index = p_other.m_index;
		}
		void move_from(choice & p_other) {
			m_data = p_other.m_data;
			m_index = std::exchange(p_other.m_index, choice_none);
		}
		template <typename T>
		void forward_from(T && p_other) {
			using type = std::remove_cv_t<T>;
			new(&m_data) T{std::forward<type>(p_other)};
			m_index = index_of<type, T_items ...>();
		}
	}; // struct

} // ns