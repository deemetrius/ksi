module;

#include "../src/pre.h"

export module just.files;

export import <filesystem>;
import <cstdio>;
export import just.text;

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
	
	using file_size_result = result<t_uint_max>;

	file_size_result file_size(const fs::path & p_path) {
		std::error_code v_error;
		std::uintmax_t v_ret = fs::file_size(p_path, v_error);
		return v_error ? file_size_result{} : file_size_result{true, v_ret};
	}
	
	struct file {
		using pointer = std::FILE *;
		
		// data
		pointer		m_handle;
		
		file(pointer p_handle) : m_handle{p_handle} {}
		file(t_plain_text p_path, t_plain_text p_mode) { fopen_s(&m_handle, p_path, p_mode); }
		~file() { if( m_handle ) { std::fclose(m_handle); } }
		
		template <typename T>
		inline t_size read(T * p_pointer, t_size p_count) {
			return std::fread(p_pointer, sizeof(T), p_count, m_handle);
		}
		
		template <typename T>
		inline t_size write(T * p_pointer, t_size p_count) {
			return std::fwrite(p_pointer, sizeof(T), p_count, m_handle);
		}
	};
	
	enum class file_read_status { nice = 0, fail_get_size, fail_open, fail_read };

	text file_read(const fs::path & p_path, file_read_status & p_status) {
		file_size_result v_size_res = just::file_size(p_path);
		if( !v_size_res ) {
			p_status = file_read_status::fail_get_size;
			return text{};
		}
		std::string v_path = p_path.string();
		file v_file{v_path.c_str(), "rb"};
		if( !v_file.m_handle ) {
			p_status = file_read_status::fail_open;
			return text{};
		}
		t_text v_text;
		text v_ret{static_cast<t_index>(v_size_res.m_value), v_text};
		t_size v_read_count = v_file.read(v_text, v_size_res.m_value);
		if( v_read_count < v_size_res.m_value ) {
			p_status = file_read_status::fail_read;
			v_ret->size(v_read_count);
		}
		v_text[v_read_count] = 0;
		p_status = file_read_status::nice;
		return v_ret;
	}

} // ns