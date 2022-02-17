module;

export module just.text.actions;
export import just.text;
import just.compare;
//import just.array;
//export import <initializer_list>;
import <compare>;
import <cstring>;
import <cwchar>;

export namespace just {

template <typename T>
concept c_text_compare_case = c_any_of<T, case_exact, case_none>;

template <typename C>
struct text_actions;

template <>
struct text_actions<char> {
	using type = char;
	using t_text = basic_text<type>;
	using pointer = t_text::pointer;
	using const_pointer = t_text::const_pointer;

	template <c_text_compare_case Case_compare = case_exact>
	static id compare(const t_text & tx1, const t_text & tx2) {
		if constexpr( std::is_same_v<Case_compare, case_exact> ) {
			return (tx1.ref_ == tx2.ref_) ? 0 : std::strcmp(tx1->cs_, tx2->cs_);
		} else {
			return (tx1.ref_ == tx2.ref_) ? 0 : _stricmp(tx1->cs_, tx2->cs_);
		}
	}

	static pointer copy_n(pointer dest, const_pointer src, id count) {
		return std::strncpy(dest, src, count);
	}
};

template <>
struct text_actions<wchar_t> {
	using type = wchar_t;
	using t_text = basic_text<type>;
	using pointer = t_text::pointer;
	using const_pointer = t_text::const_pointer;

	template <c_text_compare_case Case_compare = case_exact>
	static id compare(const t_text & tx1, const t_text & tx2) {
		if constexpr( std::is_same_v<Case_compare, case_exact> ) {
			return (tx1.ref_ == tx2.ref_) ? 0 : std::wcscmp(tx1->cs_, tx2->cs_);
		} else {
			return (tx1.ref_ == tx2.ref_) ? 0 : _wcsicmp(tx1->cs_, tx2->cs_);
		}
	}

	static pointer copy_n(pointer dest, const_pointer src, id count) {
		return std::wcsncpy(dest, src, count);
	}
};

template <c_text_compare_case Case_compare = case_exact>
struct text_case_nest {
	template <typename C, template <typename C1> typename Actions = text_actions>
	struct inner {
		using type = C;
		using t_text = basic_text<type>;
		using t_actions = Actions<type>;
		using t_ordering = compare_result; //std::strong_ordering;

		// data
		t_text text_;

		t_ordering operator <=> (const t_text & tx) const {
			//return t_actions::compare(text_, tx) <=> 0;
			return sign<t_ordering>( t_actions::template compare<Case_compare>(text_, tx) );
		}

		bool operator == (const t_text & tx) const {
			return (text_->len_ == tx->len_) && !t_actions::template compare<Case_compare>(text_, tx);
		}
	};
};

template <typename C, template <typename C1> typename Actions = text_actions>
using text_with_case = text_case_nest<case_exact>::inner<C, Actions>;

template <typename C, template <typename C1> typename Actions = text_actions>
using text_no_case = text_case_nest<case_none>::inner<C, Actions>;

//template <typename C>
/*wtext text_implode(std::initializer_list< wtext > lst) {
	using t_text = wtext;
	using t_actions = text_actions<typename t_text::type>;
	id len = 0;
	for( const t_text & it : lst ) return it;//len += it->len_;
	typename t_text::pointer s;
	t_text ret(len, s);
	for( const t_text & it : lst ) {
		t_actions::copy_n(s, it->cs_, it->len_);
		s += it->len_;
	}
	*s = 0;
	return ret;
}*/

/*struct omg {
	using type = char;
	using tt = just::basic_text<type>;

	static tt text_implode(const tt & tx) {
		using namespace text_literals;
		return "1"_jt;
		//just::array_alias<bool, just::capacity_step<3, 4> > arr;
		//return tx;
	}
};*/

} // ns just