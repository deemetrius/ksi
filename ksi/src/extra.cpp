#include "extra.h"
#include <string>
#include <sstream>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

namespace patch {

template <class T>
std::wstring to_wstring(T n) {
	std::wostringstream stm;
	stm << n;
	return stm.str();
}

} // ns

namespace ex {

mem_new::fn_re_allocate mem_new::re_allocate = mem_new::f_re_allocate;

wtext to_wtext(id num) {
	std::wstring s = patch::to_wstring(num);
	return wtext(wtext::n_copy::val, s.c_str(), s.size() );
}
wtext to_wtext(real num) {
	std::wstring s = patch::to_wstring(num);
	return wtext(wtext::n_copy::val, s.c_str(), s.size() );
}

template <class Items>
wtext inner_implode(const Items & items, const wtext & sep, id count) {
	if( !count ) return L"";
	if( count == 1 ) return *items.begin();
	id len = 0, sep_len = sep.h_->len_;
	for( const wtext & it : items )
	len += it.h_->len_;
	len += (count -1) * sep_len;
	wtext::Char * s;
	wtext tx(s = new wtext::Char[len +1], len);
	if( sep_len ) {
		bool not_first = false;
		for( const wtext & it : items ) {
			if( not_first ) {
				wcsncpy(s, sep.h_->cs_, sep_len);
				s += sep_len;
			} else
			not_first = true;
			wcsncpy(s, it.h_->cs_, it.h_->len_);
			s += it.h_->len_;
		}
	} else for( const wtext & it : items ) {
		wcsncpy(s, it.h_->cs_, it.h_->len_);
		s += it.h_->len_;
	}
	*s = 0;
	tx.h_->calc_len();
	return tx;
}

wtext implode(std::initializer_list<wtext> lst, const wtext & sep) {
	return inner_implode(lst, sep, lst.size() );
}

wtext implode(const wtext_array & items, const wtext & sep) {
	return inner_implode(items, sep, items.count_);
}

wtext replace_filename(const wtext & path, const wtext & file) {
	fs::path p(path.h_->cs_);
	p.replace_filename(file.h_->cs_);
	return wtext(wtext::n_copy::val, p.native().c_str(), p.native().size() );
}

wtext absolute_path(const wtext & path) {
	fs::path p(path.h_->cs_);
	std::error_code ec;
	fs::path a = fs::canonical(p, ec);
	return wtext(wtext::n_copy::val, a.native().c_str(), a.native().size() );
}

bool read_file(const wtext & path, text & r, id & len) {
	bool ret = false;
	fs::path p(path.h_->cs_);
	std::ifstream in(p, std::ios::binary);
	if( in.is_open() ) {
		in.seekg(0, std::ios::end);
		len = in.tellg();
		if( len >= 0 ) {
			in.seekg(0);
			if( len ) {
				char * s = new char[len +1];
				in.read(s, len);
				s[len] = 0;
				r = text(s, len);
			}
			ret = true;
		}
		in.close();
	}
	return ret;
}

} // ns
