module;

#include "../src/pre.h"

export module just.choice;
export import <type_traits>;
export import <algorithm>;
export import <utility>;
import just.common;

export namespace just {

	template <typename T, typename T_first, typename ... T_rest>
	constexpr t_index index_of(t_index p_from = 0) {
		if constexpr( std::is_same_v<T, T_first> ) { return p_from; }
		if constexpr ( sizeof...(T_rest) == 0 ) { return -1; }
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
	
	} // ns

	template <t_index C_pos, typename ... T_items>
	using choice_at_t = detail::choice_at<C_pos, T_items ...>::type;

	template <typename ... T_items>
	struct choice {
		static constexpr t_size
		c_max_size = std::max<t_size>({sizeof(T_items) ...}),
		c_max_align = std::max<t_size>({alignof(T_items) ...});
		using t_data = std::aligned_storage_t<c_max_size, c_max_align>;

		template <c_any_of<T_items...> T>
		static void deleter(t_data * p_data) { reinterpret_cast<T *>(p_data)->~T(); }

		template <c_any_of<T_items...> T>
		static void initer(t_data * p_data, const choice & p_other) {
			new (p_data) T(*reinterpret_cast<T *>(&p_other->m_data));
		}

		static constexpr decltype(&deleter<choice_at_t<0, T_items ...>>) s_deleters[sizeof...(T_items)]{&deleter<T_items> ...};
		static constexpr decltype(&initer<choice_at_t<0, T_items ...>>) s_initers[sizeof...(T_items)]{&initers<T_items> ...};

		// data
		t_data		m_data;
		t_index		m_index = -1;

		choice() = default;
		~choice() { clear(); }

		template <c_any_of<T_items...> T, typename ... T_args>
		choice(std::in_place_type_t<T>, T_args && ... p_args) {
			new(&m_data) T{std::forward<T_args>(p_args) ...};
			m_index = index_of<T, T_items ...>();
		}

		template <c_any_of<T_items...> T>
		choice(const T & p_other) { // copy
			new(&m_data) T{p_other};
			m_index = index_of<T, T_items ...>();
		}
		template <c_any_of<T_items...> T>
		choice(T && p_other) { // move
			new(&m_data) T{std::move(p_other)};
			m_index = index_of<T, T_items ...>();
		}

		template <c_any_of<T_items...> T>
		choice & operator = (const T & p_other) { // copy
			clear();
			new(&m_data) T{p_other};
			m_index = index_of<T, T_items ...>();
			return *this;
		}
		template <c_any_of<T_items...> T>
		choice & operator = (T && p_other) { // move
			clear();
			new(&m_data) T{std::move(p_other)};
			m_index = index_of<T, T_items ...>();
			return *this;
		}

		choice(const choice & p_other) { // copy
			s_initers[p_other.m_index](&m_data, p_other);
			m_index = p_other.m_index;
		}
		choice(choice && p_other) { // move
			m_data = p_other.m_data;
			m_index = std::exchange(p_other.m_index, -1);
		}

		choice & operator = (const choice & p_other) { // copy
			clear();
			s_initers[p_other.m_index](&m_data, p_other);
			m_index = p_other.m_index;
			return *this;
		}
		choice & operator = (choice && p_other) { // move
			clear();
			m_data = p_other.m_data;
			m_index = std::exchange(p_other.m_index, -1);
			return *this;
		}

		void clear() {
			if( m_index > 0 ) { s_deleters[m_index](&m_data); m_index = -1; }
		}

		template <c_any_of<T_items...> T>
		T & get() {
			return *static_cast<T *>(&m_data);
		}
	};

} // ns