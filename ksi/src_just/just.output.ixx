module;

#include "../src/pre.h"

export module just.output;

import <cstdio>;
import <cwchar>;
export import just.common;

export namespace just {
	
	struct output_base {
		// data
		t_int_ptr	m_errors = 0;
		
		operator bool () const {
			return m_errors;
		}

		virtual bool write(char p_value)				{ return false; }
		virtual bool write(wchar_t p_value)				{ return false; }
		virtual bool write(t_plain_text p_value)		{ return false; }
		virtual bool write(t_plain_text_wide p_value)	{ return false; }
		virtual bool write(int p_value)					{ return false; }
		virtual bool write(unsigned int p_value)		{ return false; }
		virtual bool write(t_diff p_value)				{ return false; }
		virtual bool write(t_size p_value)				{ return false; }
		virtual bool write(double p_value)				{ return false; }
		virtual bool write(long double p_value)			{ return false; }
	};
	
	struct output_file :
		public output_base
	{
		using t_handle = std::FILE *;

		// data
		t_handle	m_handle = stdout;

		bool write(char p_value) override {
			if( std::fputc(p_value, m_handle) == EOF ) {
				++m_errors;
				return true;
			}
			return false;
		}

		bool write(wchar_t p_value) override {
			if( std::fputwc(p_value, m_handle) == EOF ) {
				++m_errors;
				return true;
			}
			return false;
		}

		bool write(t_plain_text p_value) override {
			if( std::fputs(p_value, m_handle) == EOF ) {
				++m_errors;
				return true;
			}
			return false;
		}

		bool write(t_plain_text_wide p_value) override {
			if( std::fputws(p_value, m_handle) == EOF ) {
				++m_errors;
				return true;
			}
			return false;
		}

		bool write(int p_value) override {
			if( std::fprintf(m_handle, "%d", p_value) < 0 ) {
				++m_errors;
				return true;
			}
			return false;
		}

		bool write(unsigned int p_value) override {
			if( std::fprintf(m_handle, "%u", p_value) < 0 ) {
				++m_errors;
				return true;
			}
			return false;
		}

		bool write(t_diff p_value) override {
			if( std::fprintf(m_handle, "%td", p_value) < 0 ) {
				++m_errors;
				return true;
			}
			return false;
		}

		bool write(t_size p_value) override {
			if( std::fprintf(m_handle, "%zu", p_value) < 0 ) {
				++m_errors;
				return true;
			}
			return false;
		}

		bool write(double p_value) override {
			if( std::fprintf(m_handle, "%g", p_value) < 0 ) {
				++m_errors;
				return true;
			}
			return false;
		}

		bool write(long double p_value) override {
			if( std::fprintf(m_handle, "%Lg", p_value) < 0 ) {
				++m_errors;
				return true;
			}
			return false;
		}
	};

	output_file g_console;
	char g_new_line = '\n';
	
	template <c_any_of<
		char, wchar_t,
		t_plain_text, t_plain_text_wide,
		t_text, t_text_wide,
		int, unsigned int,
		t_diff, t_size,
		double, long double
	> T>
	output_base & operator , (output_base & p_out, T p_value) {
		p_out.write(p_value);
		return p_out;
	}

	template <typename T>
	constexpr bool g_always_false = false;

} // ns