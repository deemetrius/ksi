module;

#include "../src/pre.h"

export module just.keep;

export import just.choice;

export namespace just {

	namespace detail {

		template <typename T_base, typename T>
		struct keep_traits {
			static T_base * initer(t_byte_under * p_to, const t_byte_under * p_from) {
				return new (p_to) T(*reinterpret_cast<const T *>(p_from));
			}
		};
	
	} // ns

	template <typename T_base, std::derived_from<T_base> ... T_items>
	struct keep {
		using pointer = T_base *;

		static constexpr t_size
			c_max_size = std::max<t_size>({sizeof(T_items) ...}),
			c_max_align = std::max<t_size>({alignof(T_items) ...});

		static constexpr decltype(&detail::choice_traits< choice_at_t<0, T_items ...> >::deleter)
			s_deleters[sizeof...(T_items)]{&detail::choice_traits<T_items>::deleter ...};

		static constexpr decltype(&detail::keep_traits<T_base, choice_at_t<0, T_items ...> >::initer)
			s_initers[sizeof...(T_items)]{&detail::keep_traits<T_items>::initer ...};

		// data
		alignas(c_max_align) t_byte_under	m_data[c_max_size];
		t_index								m_index = choice_none;
		pointer								m_base = nullptr;

		void clear() {
			if( m_index > 0 ) { s_deleters[m_index](&m_data); m_index = choice_none; m_base = nullptr; }
		}

		template <c_any_of<T_items...> T>
		T & get() { return *static_cast<T *>(&m_data); }

		operator bool () const { return m_index != choice_none; }

		keep() = default;
		~keep() { clear(); }

		// in_place
		template <c_any_of<T_items...> T, typename ... T_args>
		keep(std::in_place_type_t<T>, T_args && ... p_args) {
			m_base = new(&m_data) T{std::forward<T_args>(p_args) ...};
			m_index = index_of<T, T_items ...>();
		}

		// T
		template <c_any_of<T_items...> T>
		keep(const T & p_other) { // copy
			forward_from(p_other);
		}
		template <c_any_of<T_items...> T>
		keep(T && p_other) { // move
			forward_from(p_other);
		}
		template <c_any_of<T_items...> T>
		keep & operator = (const T & p_other) { // copy
			clear();
			forward_from(p_other);
			return *this;
		}
		template <c_any_of<T_items...> T>
		keep & operator = (T && p_other) { // move
			clear();
			forward_from(p_other);
			return *this;
		}

		// keep
		keep(const keep & p_other) { // copy
			copy_from(p_other);
		}
		keep(keep && p_other) { // move
			move_from(p_other);
		}
		keep & operator = (const keep & p_other) { // copy
			clear();
			copy_from(p_other);
			return *this;
		}
		keep & operator = (keep && p_other) { // move
			clear();
			move_from(p_other);
			return *this;
		}

	private:
		void copy_from(const keep & p_other) {
			m_base = s_initers[p_other.m_index](&m_data, &p_other.m_data);
			m_index = p_other.m_index;
		}
		void move_from(keep & p_other) {
			m_data = p_other.m_data;
			m_index = std::exchange(p_other.m_index, choice_none);
			m_base = std::exchange(p_other.m_base, nullptr);
		}
		template <typename T>
		void forward_from(T && p_other) {
			using type = std::remove_cv_t<T>;
			m_base = new(&m_data) T{std::forward<type>(p_other)};
			m_index = index_of<type, T_items ...>();
		}
	}; // struct

} // ns