module;

#include "../src/pre.h"

export module just.args;

import <utility>;

export namespace just {

	template <typename T>
	concept c_not_ref = ! std::is_reference_v <T>;

	/*template <typename T>
	struct ref {
		using t_handle = T *;
		using t_accept = T &;
		using t_read = const T &;
		using t_write = T &;

		// data
		t_handle	m_handle;

		ref(t_accept p_target) : m_handle{&p_target} {}

		t_read read() const { return *m_handle; }
		t_write write() { return *m_handle; }
	};*/

	template <c_not_ref T>
	using ref = T &;

	template <c_not_ref T>
	using cref = const T &;

	template <typename T>
	struct in {
		using t_data = T;
		using t_read = const T &;
		using t_write = T &;

		// data
		t_data
			m_value;

		t_read read() const { return m_value; }
		t_write write() { return m_value; }
	};

	template <typename T>
	struct in_out {
		using t_data = T;
		using t_handle = T * const;
		using t_accept = T &;
		using t_read = const T &;
		using t_write = T &;

		// data
		t_data
			m_value;
		t_handle
			m_handle;

		in_out(t_accept p_target) : m_value{std::move(p_target)}, m_handle{&p_target} {}
		~in_out() { *m_handle = std::move(m_value); }

		t_read read() const { return m_value; }
		t_write write() { return m_value; }

		in_out() = delete;
		in_out(const in_out &) = delete;
		in_out(in_out &&) = delete;
		in_out & operator = (const in_out &) = delete;
		in_out & operator = (in_out &&) = delete;
	};

} // ns