#include "ast.h"
#include "rules.h"
#include <windows.h>

namespace ksi {

std::wostream * wc;

#ifdef KSI_LIB
void init(const api * v_api) {
	wc = &std::wcout;
	var::config::init(v_api);
}

std::wostream * get_wc() {
	return wc;
}
#endif // KSI_LIB

namespace also {

ex::wtext decode(const char * str, id src_len) {
	ex::wtext ret;
	int len = MultiByteToWideChar(CP_UTF8, 0, str, src_len, nullptr, 0);
	if( len ) {
		ret = ex::wtext(new wtext::Char[len +1], len);
		len = MultiByteToWideChar(CP_UTF8, 0, str, src_len, ret.h_->s_, len);
		ret.h_->s_[len] = 0;
		ret.h_->calc_len();
	}
	return ret;
}

ex::wtext read_file(const ex::wtext & path, bool & done) {
	done = false;
	id len;
	//ex::ref<char, true> r;
	ex::text r;
	wtext ret = ex::read_file(path, r, len) ? (done = true, decode(r.h_->cs_, len) ) : L"";
	return ret;
}

void show_tokens(const t_tokens & toks) {
	std::wcout << L"tokens:" << std::endl;
	for( const tokens::base_token * it : toks )
	std::wcout << L'\t' << it->get_name() << std::endl;
}

} // ns

using ext_init_proc = bool (__cdecl *)(const var::config *);

bool load_extension(const wtext & path) {
	bool res = false;
	if( HMODULE hl = LoadLibraryW(path.h_->cs_); hl != NULL ) {
		bool need_unload = false;
		ext_init_proc fn = reinterpret_cast<ext_init_proc>( GetProcAddress(hl, "init_ksi_extension") );
		if( fn == NULL )
		need_unload = true;
		else {
			res = fn(var::hcfg);
			if( !res )
			need_unload = true;
		}
		if( need_unload )
		FreeLibrary(hl);
	}
	return res;
}

bool load_script(const wtext & path, space * spc, const run_args & ra, base_log * log) {
	bool ret = false;
	ex::wtext full_path = ex::absolute_path(path);
	if( full_path.empty() )
	log->add({L"error: File is absent.", path, {0, 0} });
	else {
		bool done = false;
		wtext tx = also::read_file(full_path, done);
		if( done ) {
			rules::info inf = {full_path};
			t_tokens toks;
			//std::wcout << tx << std::endl;
			rules::rule_start::parse(tx.h_->cs_, inf, toks, log);
			if( ra.debug_ ) {
				std::wcout << full_path << std::endl;
				also::show_tokens(toks);
			}
			if( inf.good_ ) {
				ast::prepare_data pd(spc, full_path);
				// prepare
				for( tokens::base_token * it : toks )
				it->prepare(spc, &pd, log);
				// perform
				for( tokens::base_token * it : toks )
				it->perform(spc, &pd, log);
				//
				pd.check(log);
				//
				if( !pd.error_count_ ) {
					ret = true;
					if( ra.debug_ ) {
						ex::id side_pos = 0;
						for( const ksi::mod::side * sd : pd.mod_.h_->plain_.sides_ ) {
							std::wcout << L"side " << side_pos << L':' << std::endl;
							++side_pos;
							for( const ksi::mod::instr & it : sd->instructions_ )
							std::wcout << L'\t' << it << std::endl;
						}
						std::wcout << std::endl;
					}
					//pd.put_mod(spc);
					//pd.put_fn_all(spc);
					pd.put(spc, log);
				}
			}
		} else
		log->add({L"error: Unable to open file.", full_path, {0, 0} });
	}
	return ret;
}

bool run_script(const wtext & path, const run_args & ra, base_log * log) {
	bool ret = false;
	try {
		space spc;
		if( load_script(path, &spc, ra, log) ) {
			ret = true;
			t_stack stk;
			spc.first_run(nullptr, &stk, {0, 0}, log);
			if( ra.debug_ ) {
				*wc << std::endl
					<< L"stack size: " << stk.items_.count_
					<< L", max: " << stk.items_.size_ << std::endl
				;
				if( ra.show_log_ )
				log->out(*wc);
			} else if( ra.show_log_ && log->filled_ ) {
				*wc << std::endl;
				log->out(*wc);
			}
		} else
		log->out(*wc);
	} catch( const std::bad_alloc & e ) {
		*wc << "Memory allocation error: " << e.what() << std::endl;
	} catch( ... ) {
		*wc << L"Unknown error." << std::endl;
	}
	return ret;
}

} // ns
