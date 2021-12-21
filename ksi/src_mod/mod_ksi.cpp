// Include the required headers from httpd
#include "../src_apache/httpd.h"
#include "../src_apache/http_core.h"
#include "../src_apache/http_log.h"
#include "../src_apache/http_protocol.h"
#include "../src_apache/http_request.h"
//#include "../src_apache/apr_strings.h" // ..\src_apache\lib\libapr-1.lib // -L"D:\prog\VertrigoServ\Apache\lib"
// ..\src_apache\lib\aprutil-1.lib
#include "../src_exe/ksi_lib_loader.h"
#include "../src/ksi_types.h"
#include <iostream>
#include <cstring>
#include <new>

#define KSI_LOG_MARK __FILE__,__LINE__,ksi_module.module_index

extern "C" {
// Define prototypes of our functions in this module
static void register_hooks(apr_pool_t * pool);
static int handler_ksi(request_rec * r);
static int handler_ksi_bad(request_rec * r);

// Define our module as an entity and assign a function for registering hooks
module AP_MODULE_DECLARE_DATA ksi_module = {
	STANDARD20_MODULE_STUFF,
	NULL,            // Per-directory configuration handler
	NULL,            // Merge handler for per-directory configurations
	NULL,            // Per-server configuration handler
	NULL,            // Merge handler for per-server configurations
	NULL,            // Any directives we may have for httpd
	register_hooks   // Our hook registering function
};
} // extern

struct ksi_keep {
	static const ksi::api * api;

	static ex::wtext conv(const ex::text & tx) {
		return api->fn_decode_(tx.h_->cs_, tx.h_->len_);
	}

	static ex::wtext conv(const char * str) {
		return api->fn_decode_(str, std::strlen(str) );
	}

	static ex::wtext conv_maybe(const char * str, const ex::wtext & def) {
		return str ? conv(str) : def;
	}

	static void fill_request_info(ksi::request_info & ri, request_rec * r) {
		ri.method_		= conv(r->method);
		ri.protocol_	= conv(r->protocol);
		ri.host_		= conv(r->hostname);
		request_rec * rr = r;
		while( rr->prev ) rr = rr->prev;
		ex::text
		u_scheme(ex::base_text::n_get::val, ap_http_scheme(r) ),
		u_host(ex::base_text::n_get::val, r->hostname),
		u_path_q(ex::base_text::n_get::val, rr->filename),
		url = ex::implode({u_scheme, "://", u_host, u_path_q});
		ri.url_			= conv(url);
		ri.scheme_		= conv(u_scheme);
		//ri.hash_		= conv(rr->the_request);
		apr_uri_t uri;
		if( apr_uri_parse(r->pool, url.h_->cs_, &uri) == APR_SUCCESS ) {
			ri.path_	= conv_maybe(uri.path, L"/"); // apr_escape_urlencoded
			ri.query_	= conv_maybe(uri.query, L"");
		}
	}
};
const ksi::api * ksi_keep::api = nullptr;

struct custom_streambuf : public std::wstreambuf {
	request_rec * r_;

	custom_streambuf(request_rec * r) : r_(r) {}

	std::streamsize xsputn(const char_type * str, std::streamsize count) override {
		if( ex::text tx = ksi_keep::api->fn_encode_(str, count) ) {
			ap_rputs(tx.h_->cs_, r_);
		}
		return count;
	}

	/*int sync() override {
		return 0;
	}*/
};

struct log_custom : public ksi::base_log {
	request_rec * r_;

	log_custom(request_rec * r) : r_(r) {}

	void add(const ksi::also::message & msg) {
		ex::wtext wtx = ex::implode({
			L"\"", msg.file_, L"\" [", ex::to_wtext(msg.pos_.line_), L", ", ex::to_wtext(msg.pos_.col_), L"] ", msg.msg_
		});
		ex::text tx = ksi_keep::api->fn_encode_(wtx.h_->cs_, wtx.h_->len_);
		ap_log_rerror(KSI_LOG_MARK, APLOG_WARNING, 0, r_, "%s", tx.h_->cs_);
	}
};

template <std::size_t N>
bool is_ends_with(const char * str, const char (&end)[N]) {
	std::size_t len = std::strlen(str);
	if( len < N -1 ) return false;
	return !std::strncmp(str + len - N +1, end, N -1);
}

template <class T>
struct later {
	T * fn_;

	~later() {
		(*fn_)();
	}
};

// register_hooks: Adds a hook to the httpd process
static void register_hooks(apr_pool_t * pool) {
	if(( ksi_keep::api = ksi::lib_loader::load_and_get_api(L"mod_ksi") )) {
		// Hook the request handler
		ap_hook_handler(handler_ksi, NULL, NULL, APR_HOOK_LAST);
	} else {
		ap_hook_handler(handler_ksi_bad, NULL, NULL, APR_HOOK_LAST);
	}
}

int handler_ksi_bad(request_rec * r) {
	if( !is_ends_with(r->path_info, ".ksi") ) return (DECLINED);
	static bool first_run = true;
	if( first_run ) {
		first_run = false;
		ap_log_rerror(KSI_LOG_MARK, APLOG_ERR, 0, r, "Unable to load ksi library.");
	}
	return HTTP_INTERNAL_SERVER_ERROR;
}

// The handler function for our module.
static int handler_ksi(request_rec * r) {
	/* First off, we need to check if this is a call for the "example" handler.
	 * If it is, we accept it and do our things, it not, we simply return DECLINED,
	 * and Apache will try somewhere else.
	 */
	//if (!r->handler || strcmp(r->handler, "ksi-script")) return (DECLINED);
	if( !is_ends_with(r->path_info, ".ksi") ) return (DECLINED);
	//
	{
		apr_finfo_t finfo;
		int rc = apr_stat(&finfo, r->path_info, APR_FINFO_MIN, r->pool);
		if( rc == APR_SUCCESS ) {
			bool exists = (finfo.filetype != APR_NOFILE) && !(finfo.filetype & APR_DIR);
			if( !exists ) return HTTP_NOT_FOUND; // Return a 404 if not found.
		}
		else return HTTP_FORBIDDEN;
	}
	try {
		std::wostream * wo = ksi_keep::api->fn_get_wc_();
		std::wstreambuf * buf_orig = wo->rdbuf();
		custom_streambuf buf_custom(r);
		auto lambda = [wo, buf_orig](){ try { wo->rdbuf(buf_orig); } catch( ... ) {} };
		later<decltype(lambda)> guard{&lambda};
		wo->rdbuf(&buf_custom);
		//
		ap_set_content_type(r, "text/html");
		ksi::request_info req_inf;
		ksi_keep::fill_request_info(req_inf, r);
		ksi::run_args ra;
		ra.req_inf_ = &req_inf;
		log_custom log(r);
		ex::wtext path = ksi_keep::api->fn_decode_(r->path_info, std::strlen(r->path_info) );
		ksi_keep::api->fn_run_script_(path, ra, &log);
		/*
		*wo << L"Test\n";
		ap_rputs(r->filename, r); ap_rputs("\n", r);
		ap_rputs(r->canonical_filename, r); ap_rputs("\n", r);
		ap_rputs(r->the_request, r); ap_rputs("\n", r);
		ap_rputs(r->hostname, r); ap_rputs("\n", r);
		ap_rputs(r->path_info, r); ap_rputs("\n", r);
		*/
		//wo->rdbuf(buf_orig);
	} catch( const std::bad_alloc & e ) {
		ap_log_rerror(KSI_LOG_MARK, APLOG_ERR, 0, r, "Memory allocation exception was thrown.");
	} catch( ... ) {
		ap_log_rerror(KSI_LOG_MARK, APLOG_ERR, 0, r, "Unhandled exception was thrown.");
	}
	return OK;
}
