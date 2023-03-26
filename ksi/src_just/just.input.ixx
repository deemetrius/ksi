module;

#include "../src/pre.h"

export module just.input;

export import <filesystem>;
export import <fstream>;
import <cstdlib>;

export namespace just {

	namespace fs = std::filesystem;

	struct open_file_error {};

	std::string read_file(const fs::path & p_path) {
		std::ifstream v_file{p_path, std::ios_base::in | std::ios_base::binary | std::ios_base::ate};
		if( ! v_file.is_open() ) { throw open_file_error{}; }
		v_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		std::ifstream::pos_type v_size = v_file.tellg();
		v_file.seekg(0);
		std::string ret(v_size, '\0');
		v_file.read(ret.data(), v_size);
		ret.resize( v_file.gcount() );
		return ret;
	}

	struct conversion_error {};

	std::wstring to_wide(const std::string & p) {
		std::size_t v_size;
		if(
			mbstowcs_s(&v_size, nullptr, 0, p.data(), p.size() )
		) {
			throw conversion_error{};
		}
		std::wstring ret(v_size, L'\0');
		if(
			mbstowcs_s(&v_size, ret.data(), v_size, p.data(), p.size() )
		) {
			throw conversion_error{};
		}
		ret.resize(v_size);
		return ret;
	}

	std::wstring read_wide(const fs::path & p_path) {
		/*std::wifstream v_file{p_path, std::ios_base::in | std::ios_base::binary | std::ios_base::ate};
		if( ! v_file.is_open() ) { throw open_file_error{}; }
		v_file.exceptions(std::wifstream::failbit | std::wifstream::badbit);
		std::wifstream::pos_type v_size = v_file.tellg();
		v_file.seekg(0);
		std::wstring ret(v_size, '\0');
		v_file.read(ret.data(), v_size);
		ret.resize( v_file.gcount() );
		return ret;*/
		return to_wide( read_file(p_path) );
	}

} // ns