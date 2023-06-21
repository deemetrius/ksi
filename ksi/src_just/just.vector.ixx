module;

#include "../src/pre.h"

export module just.vector;

export import just.iter;
export import <concepts>;
export import <utility>;
export import <new>;
export import <cstring>;
export import <compare>;
export import <iterator>;

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
	struct base_vector {
		using value_type = T;
		using pointer = value_type *;
		using reference = value_type &;
		using t_grow = Grow;
		using size_type = t_index;
		using difference_type = t_index;

		static constexpr std::align_val_t
			s_align{alignof(value_type)};
		static constexpr t_size
			s_size{sizeof(value_type)};

		static constexpr bool
			s_with_custom_default_constructor = ! std::is_trivially_default_constructible_v<value_type>,
			s_with_destructor = ! std::is_trivially_destructible_v<value_type>;

		struct mem {
			static pointer allocate(size_type p_reserve) {
				return static_cast<pointer>(
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

			pointer do_alloc(size_type p_reserve) const {
				pointer ret = m_alloc(p_reserve);
				if( ret == nullptr ) { throw std::bad_alloc{}; }
				return ret;
			}
		};

		static constexpr mem s_mem{};

		using mem_const_pointer = const mem *;

	protected:

		struct t_data {
			pointer
				m_handle;
			size_type
				m_reserve,
				m_count = 0;
			mem_const_pointer
				m_mem = &s_mem;
		};

		using t_data_pointer = t_data *;

		// data
		t_data
			m_data;

		union t_late {
			// data
			value_type
				m_item;

			t_late() requires (s_with_custom_default_constructor) {}
			~t_late() requires (s_with_destructor) {}
		};

	public:

		base_vector(const base_vector &) = delete;
		base_vector(base_vector &&) = delete;
		base_vector & operator = (const base_vector &) = delete;

		base_vector & operator = (base_vector && p_other) {
			std::ranges::swap(m_data, p_other.m_data);
		}

		base_vector(mem_const_pointer p_mem = &s_mem) : base_vector(t_grow::get_initial_reserve(), p_mem) {}

		base_vector(size_type p_reserve, mem_const_pointer p_mem = &s_mem) {
			m_data.m_mem = p_mem;
			m_data.m_handle = m_data.m_mem->do_alloc(p_reserve);
			m_data.m_reserve = p_reserve;
		}

		~base_vector() {
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

		size_type size() const {
			return m_data.m_count;
		}

		size_type capacity() const {
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

		void erase(size_type p_position) {
			if constexpr ( s_with_destructor ) {
				m_data.m_handle[p_position].~value_type();
			}
			if( p_position < m_data.m_count - 1 ) {
				size_type v_rest_count = m_data.m_count - p_position - 1;
				std::memmove(m_data.m_handle + p_position, m_data.m_handle + p_position + 1, v_rest_count * s_size);
			}
			--m_data.m_count;
		}

		void pop_back_n(size_type p_number) {
			if constexpr ( s_with_destructor ) {
				for( size_type vi = 0; vi < p_number; ++vi ) {
					pop_back();
				}
			} else {
				m_data.m_count -= p_number;
			}
		}

		void pop_back() {
			if constexpr ( s_with_destructor ) {
				back().~value_type();
			}
			--m_data.m_count;
		}

		// element access

		reference operator [] (size_type p_index) {
			return m_data.m_handle[p_index];
		}

		reference front() {
			return *m_data.m_handle;
		}

		reference back(size_type p_position_from_end = 0) {
			size_type v_index = m_data.m_count - p_position_from_end - 1;
			return m_data.m_handle[v_index];
		}

		// add

		template <typename ... Args>
		void emplace(size_type p_position, Args && ... p_args) {
			if( p_position >= m_data.m_count ) {
				emplace_back(std::forward<Args>(p_args) ...);
				return;
			}
			if( full() ) {
				size_type v_new_reserve = t_grow::calc_new_reserve(m_data.m_reserve, 1);
				base_vector v{v_new_reserve, m_data.m_mem};

				// construct inserted at temporary location
				t_late v_inserted;
				new (&v_inserted.m_item) value_type{std::forward<Args>(p_args) ...};

				// copy items before position
				if( p_position > 0 ) {
					std::memcpy(v.m_data.m_handle, m_data.m_handle, p_position * s_size);
				}

				// copy inserted
				std::memcpy(v.m_data.m_handle + p_position, &v_inserted.m_item, s_size);

				// copy rest items
				size_type v_rest_count = m_data.m_count - p_position;
				std::memcpy(v.m_data.m_handle + p_position + 1, m_data.m_handle + p_position, v_rest_count * s_size);

				// update counts and swap
				v.m_data.m_count = std::exchange(m_data.m_count, 0) + 1;
				std::ranges::swap(m_data, v.m_data);
			} else {
				// construct inserted at temporary location
				t_late v_inserted;
				new (&v_inserted.m_item) value_type{std::forward<Args>(p_args) ...};

				// move rest items
				size_type v_rest_count = m_data.m_count - p_position;
				std::memmove(m_data.m_handle + p_position + 1, m_data.m_handle + p_position, v_rest_count * s_size);

				// copy inserted
				std::memcpy(m_data.m_handle + p_position, &v_inserted.m_item, s_size);

				++m_data.m_count;
			}
		}

		template <typename ... Args>
		void emplace_back(Args && ... p_args) {
			if( full() ) {
				size_type v_new_reserve = t_grow::calc_new_reserve(m_data.m_reserve, 1);
				base_vector v{v_new_reserve, m_data.m_mem};
				std::memcpy(v.m_data.m_handle, m_data.m_handle, m_data.m_count * s_size);
				std::ranges::swap(m_data.m_count, v.m_data.m_count);
				std::ranges::swap(m_data, v.m_data);
			}
			new (m_data.m_handle + m_data.m_count) value_type{std::forward<Args>(p_args) ...};
			++m_data.m_count;
		}
	};

	template <typename T, typename Grow = grow_step<> >
	struct vector : public base_vector<T, Grow> {
		using base = base_vector<T, Grow>;
		using base::base;

		template <direction Direction>
		struct t_iterator {
			using iterator_category = std::random_access_iterator_tag;
			using value_type = base::value_type;
			using pointer = base::pointer;
			using reference = base::reference;
			using difference_type = base::difference_type;

			// data
			base::t_data_pointer
				m_handle;
			difference_type
				m_index;

			// indirection

			reference operator * () { return m_handle->m_handle[m_index]; }
			pointer operator -> () { return m_handle->m_handle + m_index; }

			// arithmetic

			t_iterator & operator ++ () {
				if constexpr ( Direction == direction::forward ) {
					++m_index;
				} else {
					--m_index;
				}
				return *this;
			}

			t_iterator & operator -- () {
				if constexpr ( Direction == direction::forward ) {
					--m_index;
				} else {
					++m_index;
				}
				return *this;
			}

			t_iterator & operator += (difference_type p) {
				if constexpr ( Direction == direction::forward ) {
					m_index += p;
				} else {
					m_index -= p;
				}
				return *this;
			}

			t_iterator & operator -= (difference_type p) {
				if constexpr ( Direction == direction::forward ) {
					m_index -= p;
				} else {
					m_index += p;
				}
				return *this;
			}

			friend t_iterator operator + (const t_iterator & p_it, difference_type p) {
				if constexpr ( Direction == direction::forward ) {
					return t_iterator{p_it.m_handle, p_it.m_index + p};
				} else {
					return t_iterator{p_it.m_handle, p_it.m_index - p};
				}
			}

			friend t_iterator operator + (difference_type p, const t_iterator & p_it) {
				return p_it + p;
			}

			friend t_iterator operator - (const t_iterator & p_it, difference_type p) {
				if constexpr ( Direction == direction::forward ) {
					return t_iterator{p_it.m_handle, p_it.m_index - p};
				} else {
					return t_iterator{p_it.m_handle, p_it.m_index + p};
				}
			}

			friend t_iterator operator - (difference_type p, const t_iterator & p_it) {
				return p_it - p;
			}

			// comparison

			bool operator == (sentinel_type) const {
				if constexpr ( Direction == direction::forward ) {
					return m_index >= m_handle->m_count;
				} else {
					return m_index < 0;
				}
			}

			friend bool operator == (sentinel_type p_sentinel, const t_iterator & p_it) {
				return p_it == p_sentinel;
			}

			bool operator == (const t_iterator & p_other) const {
				return m_index == p_other.m_index;
			}

			std::strong_ordering operator <=> (const t_iterator & p_other) const {
				return m_index <=> p_other.m_index;
			}
		};

		using iterator = t_iterator<direction::forward>;
		using reverse_iterator = t_iterator<direction::reverse>;

		// iteration

		iterator begin() { return iterator{&this->m_data, 0}; }
		sentinel_type end() { return {}; }

		range_for<iterator, sentinel_type> range(base::size_type p_from) {
			return {{&this->m_data, p_from}};
		}

		range_for<iterator, iterator> range(base::size_type p_from, base::size_type p_count) {
			return {{&this->m_data, p_from}, {&this->m_data, p_from + p_count}};
		}

		// reverse iteration

		range_for<reverse_iterator, sentinel_type> reverse_range() {
			return {{&this->m_data, this->m_data.m_count - 1}};
		}

		range_for<reverse_iterator, reverse_iterator> reverse_range(base::size_type p_till) {
			return {{&this->m_data, this->m_data.m_count - 1}, {&this->m_data, p_till - 1}};
		}

		range_for<reverse_iterator, reverse_iterator> reverse_range(base::size_type p_from, base::size_type p_count) {
			return {{&this->m_data, p_from}, {&this->m_data, p_from - p_count}};
		}
	};

} //