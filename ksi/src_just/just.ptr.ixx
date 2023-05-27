module;

#include "../src/pre.h"

export module just.ptr;

export import <utility>;
export import <concepts>;

export namespace just {

	template <typename T, std::derived_from<T> Target>
	struct traits_ptr {
		using pointer = T *;
		using target_pointer = Target *;

		static void close(pointer p_handle) { delete static_cast<target_pointer>(p_handle); }
	};

	template <typename T>
	struct ptr {
		using element_type = T;
		using pointer = T *;
		using reference = T &;
		using deleter_type = decltype(&traits_ptr<T, T>::close);

	private:
		struct data {
			static void no_close(pointer) {}

			// data
			pointer
				m_handle = nullptr;
			deleter_type
				m_close = &no_close;

			~data() { close(); }

			void close() {
				m_close(m_handle);
			}

			void unset() {
				m_handle = nullptr;
				m_close = &no_close;
			}
		};

		// data
		data
			m_data;

	public:
		ptr() = default;

		// no copy
		ptr(const ptr &) = delete;
		ptr & operator = (const ptr &) = delete;

		// in_place
		template <std::derived_from<T> Target, typename ... Args>
		ptr(std::in_place_type_t<Target>, Args && ... p_args) {
			m_data.m_handle = new Target{std::forward<Args>(p_args) ...};
			m_data.m_close = &traits_ptr<T, Target>::close;
		}

		template <std::derived_from<T> Target, typename ... Args>
		void set(std::in_place_type_t<Target>, Args && ... p_args) {
			m_data.close();
			m_data.m_handle = new Target{std::forward<Args>(p_args) ...};
			m_data.m_close = &traits_ptr<T, Target>::close;
		}

		// move
		ptr(ptr && p_other) { std::swap(m_data, p_other.m_data); }
		ptr & operator = (ptr && p_other) {
			std::swap(m_data, p_other.m_data);
			return *this;
		}

		void reset() {
			m_data.close();
			m_data.unset();
		}

		pointer get() { return m_data.m_handle; }
		reference operator * () { return *get(); }
		pointer operator -> () { return get(); }

		bool empty() const { return m_data.m_handle == nullptr; }
	};

} // ns