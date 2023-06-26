module;

#include "../src/pre.h"

export module ksi.undefined_yet;

export import ksi.space;

export namespace ksi {

	struct undefined_yet {
	
		struct unknown_property {
			using t_items = just::vector<act::action::pointer, just::grow_step<8, 7> >;

			// data
			t_items
				m_items;
		};

		struct per_module {
			using t_items = std::map<text_str, unknown_property, std::ranges::less>;

			// data
			t_items
				m_items;
		};

		using t_items = std::map<text_str, per_module, std::ranges::less>;

		// data
		t_items
			m_items;

	};

} // ns