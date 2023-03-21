module;

#include "../src/pre.h"

export module ksi.var:types;

export import just.std;
export import just.output;
export import just.text;
export import <variant>;

export namespace ksi {

	using t_integer = just::t_integer;
	using t_floating = double;

	using text_str = just::text_str;
	using t_char = text_str::value_type;
	using tc_char_pointer = text_str::const_pointer;

	using t_text = just::text;

	namespace var {

		enum class t_compare { less = -1, equal = 0, greater = +1 };

		just::output_base & operator << (just::output_base & po, t_compare p) {
			po << (p == t_compare::less ? "less" : (p == t_compare::equal ? "equal" : "greater"));
			return po;
		}

		struct with_id_name {
			// data
			t_integer
				m_id;
			t_text
				m_name;
		};

		struct category : public with_id_name {
			using pointer = category *;
		};

		struct type : public with_id_name {
			using pointer = type *;

			// data
			bool
				is_map_key = false;
		};

		struct marker_array {};
		struct marker_map {};

	} // ns

	using t_cat = var::category::pointer;
	using t_type = var::type::pointer;
	using t_array = var::marker_array *;
	using t_map = var::marker_map *;

	namespace var {
	
		using t_variant = std::variant<
			t_cat,
			t_type,
			bool,
			t_integer,
			t_floating,
			t_text,
			t_array,
			t_map
		>;
	
	} // ns

} // ns