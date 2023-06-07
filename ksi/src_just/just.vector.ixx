module;

#include "../src/pre.h"

export module just.vector;

export import just.std;
export import <concepts>;
export import <utility>;
export import <new>;
export import <cstring>;

export namespace just {

	template <t_index Initial = 16, t_index Step = 7>
	struct grow_step {
		static constexpr t_index
			s_initial = Initial,
			s_step = Step;

		static constexpr t_index get_initial_reserve() {
			return s_initial;
		}

		static constexpr t_index calc_new_reserve(t_index p_current_reserve, t_index p_going_to_add) {
			return p_current_reserve + p_going_to_add + s_step;
		}
	};

	template <typename T, typename Grow = grow_step<> >
	struct vector {
		using element_type = T;
		using pointer = element_type *;
		using reference = element_type &;
		using t_grow = Grow;

		static constexpr std::align_val_t
			s_align{alignof(element_type)};
		static constexpr t_size
			s_size{sizeof(element_type)};

		static constexpr bool
			s_with_destructor = ! std::is_trivially_destructible_v<element_type>;

		struct mem {
			static pointer allocate(t_index p_reserve) {
				return reinterpret_cast<pointer>(
					::operator new(s_size * p_reserve, s_align, std::nothrow)
				);
			}

			static void free(pointer p_handle) {
				::operator delete(p_handle, s_align);
			}

			using tfn_allocate = decltype(&allocate);
			using tfn_free = decltype(&free);

			// data
			tfn_allocate
				m_alloc = &allocate;
			tfn_free
				m_free = &free;

			pointer do_alloc(t_index p_reserve) const {
				pointer ret = m_alloc(p_reserve);
				if( ret == nullptr ) { throw std::bad_alloc{}; }
				return ret;
			}
		};

		static constexpr mem s_mem{};

		using mem_const_pointer = const mem *;

	private:

		struct t_data {
			pointer
				m_handle;
			t_index
				m_reserve,
				m_count = 0;
			mem_const_pointer
				m_mem = &s_mem;
		};

		// data
		t_data
			m_data;

	public:

		vector(const vector &) = delete;
		vector(vector &&) = delete;
		vector & operator = (const vector &) = delete;

		vector & operator = (vector && p_other) {
			std::ranges::swap(m_data, p_other.m_data);
		}

		vector(mem_const_pointer p_mem = &s_mem) : vector(t_grow::get_initial_reserve(), p_mem) {}

		vector(t_index p_reserve, mem_const_pointer p_mem = &s_mem) {
			m_data.m_mem = p_mem;
			m_data.m_handle = m_data.m_mem->do_alloc(p_reserve);
			m_data.m_reserve = p_reserve;
		}

		~vector() {
			if constexpr ( s_with_destructor ) {
				clear();
			}
			m_data.m_mem->m_free(m_data.m_handle);
		}

		// info

		bool empty() const {
			return m_data.m_count == 0;
		}

		bool full() const {
			return m_data.m_count == m_data.m_reserve;
		}

		t_index size() const {
			return m_data.m_count;
		}

		t_index capacity() const {
			return m_data.m_reserve;
		}

		// erase

		void clear() {
			if constexpr ( s_with_destructor ) {
				while( m_data.m_count > 0 ) {
					pop_back();
				}
			} else {
				m_data.m_count = 0;
			}
		}

		void pop_back_n(t_index p_number) {
			if constexpr ( s_with_destructor ) {
				for( t_index vi = 0; vi < p_number; ++vi ) {
					pop_back();
				}
			} else {
				m_data.m_count -= p_number;
			}
		}

		void pop_back() {
			if constexpr ( s_with_destructor ) {
				back().~element_type();
			}
			--m_data.m_count;
		}

		// element access

		reference operator [] (t_index p_index) {
			return m_data.m_handle[p_index];
		}

		reference front() {
			return *m_data.m_handle;
		}

		reference back(t_index p_position_from_end = 0) {
			t_index v_index = m_data.m_count - p_position_from_end - 1;
			return m_data.m_handle[v_index];
		}

		// add

		template <typename ... Args>
		void emplace_back(Args && ... p_args) {
			if( full() ) {
				t_index v_new_reserve = t_grow::calc_new_reserve(m_data.m_reserve, 1);
				vector v{v_new_reserve, m_data.m_mem};
				std::memcpy(v.m_data.m_handle, m_data.m_handle, m_data.m_count * s_size);
				std::ranges::swap(m_data.m_count, v.m_data.m_count);
				std::ranges::swap(m_data, v.m_data);
			}
			new (m_data.m_handle + m_data.m_count) element_type{std::forward<Args>(p_args) ...};
			++m_data.m_count;
		}
	};


} //