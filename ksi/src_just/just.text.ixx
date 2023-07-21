module;

#include "../src/pre.h"

export module just.text;

export import just.output;
export import <memory>;
export import <string>;
export import <string_view>;
export import <initializer_list>;
import <sstream>;

export namespace just {

	using text_str = std::wstring;
	using text_view = std::wstring_view;

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

		bool is_same(const text & p_other) const { return m_ptr.get() == p_other.m_ptr.get(); }

		text_view view() { return *m_ptr.get(); }
	};

	template <typename T_container>
	text_str implode_items(const T_container & p_items, text_view p_delimiter = {}, text_view p_prefix = {}) {
		std::wostringstream ret{};
		ret << p_prefix;
		for( bool v_first = true; const text_view & it : p_items ) {
			if( v_first ) { v_first = false; } else { ret << p_delimiter; }
			ret << it;
		}
		return std::move(ret).str();
	}

	text_str implode(std::initializer_list<text_view> p_items, text_view p_delimiter = {}, text_view p_prefix = {}) {
		return implode_items(p_items, p_delimiter, p_prefix);
	}

	//

	output_base & operator << (output_base & p_out, const text & p_text) {
		p_out << p_text.m_ptr->data();
		return p_out;
	}

} // ns