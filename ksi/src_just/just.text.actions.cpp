module;

export module just.text.actions;
export import just.text;
import <compare>;
import <cwchar>;

export namespace just {

template <typename C>
struct text_actions;

template <>
struct text_actions<wchar_t> {
	using type = wchar_t;
	using t_text = basic_text<type>;

	static id compare(const t_text & tx1, const t_text & tx2) {
		return (tx1.ref_ == tx2.ref_) ? 0 : std::wcscmp(tx1->cs_, tx2->cs_);
	}

	static id compare_no_case(const t_text & tx1, const t_text & tx2) {
		return (tx1.ref_ == tx2.ref_) ? 0 : _wcsicmp(tx1->cs_, tx2->cs_);
	}
};

template <typename C>
struct text_with_case {
	using type = C;
	using t_text = basic_text<type>;

	t_text text_;

	std::strong_ordering operator <=> (const t_text & tx) const {
		const auto cmp = text_actions<type>::compare(text_, tx);
		return cmp == 0 ? std::strong_ordering::equal : (
			cmp < 0 ? std::strong_ordering::less : std::strong_ordering::greater
		);
	}

	bool operator == (const t_text & tx) const {
		return !text_actions<type>::compare(text_, tx);
	}
};

template <typename C>
struct text_no_case {
	using type = C;
	using t_text = basic_text<type>;

	t_text text_;

	std::weak_ordering operator <=> (const t_text & tx) const {
		const auto cmp = text_actions<type>::compare_no_case(text_, tx);
		return cmp == 0 ? std::strong_ordering::equivalent : (
			cmp < 0 ? std::weak_ordering::less : std::weak_ordering::greater
		);
	}

	bool operator == (const t_text & tx) const {
		return !text_actions<type>::compare_no_case(text_, tx);
	}
};

} // ns