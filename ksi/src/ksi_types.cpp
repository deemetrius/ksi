#include "ksi_types.h"
#include <fstream>
#include <ctime>
#include <iomanip>

namespace ksi {

log_file::log_file(const wtext & path) : path_(path) {
	std::wofstream fo(path_.h_->cs_, std::ios::binary | std::ios::app | std::ios::ate);
	pos_ = fo.tellp();
}

void log_file::add(const also::message & msg) {
	filled_ = true;
	std::time_t t = std::time(nullptr);
	std::tm tm = *std::localtime(&t);
	std::wofstream fo(path_.h_->cs_, std::ios::binary | std::ios::app);
	fo << std::put_time(&tm, L"%Y-%m-%d %H:%M:%S ~ ") << msg;
}

void log_file::out(std::wostream & wo) const {
	std::wifstream fi(path_.h_->cs_, std::ios::binary);
	fi.seekg(pos_);
	std::wstring s;
	while( std::getline(fi, s) ) {
		wo << s << std::endl;
	}
}

}
