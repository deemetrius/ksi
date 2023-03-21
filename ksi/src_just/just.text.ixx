module;

#include "../src/pre.h"

export module just.text;

export import <memory>;
export import <string>;
export import <initializer_list>;
import <sstream>;

export namespace just {

	using text_str = std::wstring;

	struct text {
		using t_ptr = std::shared_ptr<text_str>;
		using t_pointer = text_str *;
		using t_reference = text_str &;

		static inline t_ptr s_empty{std::make_shared<text_str>()};

		// data
		t_ptr
			m_ptr;

		text() : m_ptr{s_empty} {}

		text(text_str && p_text) : m_ptr{std::make_shared<text_str>( std::move(p_text) )} {}
		text(const text_str & p_text) : m_ptr{std::make_shared<text_str>(p_text)} {}

		text & operator = (text_str && p_text) { m_ptr = std::make_shared<text_str>( std::move(p_text) ); return *this; }
		text & operator = (const text_str & p_text) { m_ptr = std::make_shared<text_str>(p_text); return *this; }

		t_reference operator * () const { return *m_ptr.get(); }
		t_pointer operator -> () const { return m_ptr.get(); }
	};

	template <typename T_container>
	text_str implode_items(const T_container & p_items, text p_delimiter = {}, text p_prefix = {}) {
		std::wostringstream ret{};
		ret << *p_prefix;
		for( bool v_first = true; const text & it : p_items ) {
			if( v_first ) { v_first = false; } else { ret << *p_delimiter; }
			ret << *it;
		}
		return ret.str();
	}

	text_str implode(std::initializer_list<text> p_items, text p_delimiter = {}, text p_prefix = {}) {
		return implode_items(p_items, p_delimiter, p_prefix);
	}

} // ns