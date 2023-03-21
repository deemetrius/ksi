module;

#include "../src/pre.h"

export module just.output;

export import just.std;
import <type_traits>;
import <cstdio>;
//import <cwchar>;
import <limits>;
import <cmath>;

export namespace just {
	
	namespace detail {
	
		struct output_base {
			virtual bool write(t_integer p_value) { return false; }
		};
	
	} // ns

	struct output_base :
		public std::conditional_t<g_index_is_integer, none, detail::output_base>
	{
		// data
		t_integer
			m_errors = 0;
		
		operator bool () const {
			return m_errors;
		}

		virtual bool write(char p_value)				{ return false; }
		virtual bool write(wchar_t p_value)				{ return false; }
		virtual bool write(t_const_text p_value)		{ return false; }
		virtual bool write(t_const_text_wide p_value)	{ return false; }
		virtual bool write(t_index p_value)				{ return false; }
		virtual bool write(t_size p_value)				{ return false; }
		virtual bool write(double p_value)				{ return false; }
		virtual bool write(long double p_value)			{ return false; }
	};
	
	namespace detail {
	
		template <typename T_target, typename T_base>
		struct output_file :
			public T_base
		{
			bool write(t_integer p_value) override {
				T_target * v = static_cast<T_target *>(this);
				if( std::fprintf(v->m_handle, "%zd", p_value) < 0 ) {
					++v->m_errors;
					return true;
				}
				return false;
			}
		};
	
	} // ns

	struct output_file :
		public std::conditional_t<g_index_is_integer, output_base, detail::output_file<output_file, output_base> >
	{
		using t_handle = std::FILE *;
		using t_limits_double = std::numeric_limits<double>;
		using t_limits_double_long = std::numeric_limits<long double>;

		static constexpr int s_digits_double = t_limits_double::digits10;
		static constexpr int s_digits_double_long = t_limits_double_long::digits10;

		// data
		t_handle
			m_handle = stdout;

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

		bool write(t_const_text p_value) override {
			if( std::fputs(p_value, m_handle) == EOF ) {
				++m_errors;
				return true;
			}
			return false;
		}

		bool write(t_const_text_wide p_value) override {
			if( std::fputws(p_value, m_handle) == EOF ) {
				++m_errors;
				return true;
			}
			return false;
		}

		bool write(t_index p_value) override {
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
			if( std::isnan(p_value) ) {
				if( std::fputs("NaN", m_handle) == EOF ) {
					++m_errors;
					return true;
				}
				return false;
			}
			if( std::isinf(p_value) ) {
				if( std::fputs(((p_value > 0.0) ? "infinity" : "-infinity"), m_handle) == EOF ) {
					++m_errors;
					return true;
				}
				return false;
			}
			if( std::fprintf(m_handle, "%.*g", s_digits_double, p_value) < 0 ) {
				++m_errors;
				return true;
			}
			return false;
		}

		bool write(long double p_value) override {
			if( std::isnan(p_value) ) {
				if( std::fputs("NaN", m_handle) == EOF ) {
					++m_errors;
					return true;
				}
				return false;
			}
			if( std::isinf(p_value) ) {
				if( std::fputs(((p_value > 0.0) ? "infinity" : "-infinity"), m_handle) == EOF ) {
					++m_errors;
					return true;
				}
				return false;
			}
			if( std::fprintf(m_handle, "%.*Lg", s_digits_double_long, p_value) < 0 ) {
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
		t_const_text, t_const_text_wide,
		t_text, t_text_wide,
		t_size, t_index, t_integer,
		double, long double
	> T>
	output_base & operator << (output_base & p_out, T p_value) {
		p_out.write(p_value);
		return p_out;
	}

	//template <typename T>
	//constexpr bool g_always_false = false;

} // ns