module;

#include "../src/pre.h"

export module just.files;

export import <filesystem>;
import <cstdio>;
export import just.common;

export namespace just {
	
	namespace fs = std::filesystem;
	
	template <typename T>
	struct result {
		using type = T;
		
		// data
		bool	m_nice = false;
		type	m_value;
		
		operator bool () const { return m_nice; }
		bool operator ! () const { return !m_nice; }
	};
	
	fs::file_type file_type(const fs::path & p_file) {
		//using t_result = result<fs::file_type>;
		std::error_code v_error;
		fs::file_status v_status = fs::status(p_file, v_error);
		return v_error ? fs::file_type::none : v_status.type();
	}
	
	result<t_uint_max> file_size(const fs::path & p_file) {
		using t_result = result<t_uint_max>;
		std::error_code v_error;
		std::uintmax_t v_ret = fs::file_size(p_file, v_error);
		return v_error ? t_result{} : t_result{true, v_ret};
	}
	
	struct file {
		using pointer = std::FILE *;
		
		// data
		pointer		m_handle;
		
		file(pointer p_handle) : m_handle{p_handle} {}
		~file() { std::fclose(m_handle); }
		
		template <typename T>
		inline t_size read(T * p_pointer, t_size p_count) {
			return std::fread(p_pointer, sizeof(T), p_count, m_handle);
		}
		
		template <typename T>
		inline t_size write(T * p_pointer, t_size p_count) {
			return std::fwrite(p_pointer, sizeof(T), p_count, m_handle);
		}
	};
	
} // ns