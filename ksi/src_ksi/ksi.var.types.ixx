module;

#include "../src/pre.h"

export module ksi.var:types;

export import just.std;
export import just.output;
export import just.text;
export import <vector>;
export import <set>;
export import <variant>;

export namespace ksi {

	using t_index = just::t_index;
	using t_integer = just::t_integer;
	using t_floating = double;

	using text_view = just::text_view;
	using text_str = just::text_str;
	using text_char = text_str::value_type;
	using text_raw = text_str::const_pointer;

	using t_text = just::text;

	namespace var {

		enum class t_compare { less = -1, equal = 0, greater = +1 };

		just::output_base & operator << (just::output_base & po, t_compare p) {
			po << (p == t_compare::less ? "less" : (p == t_compare::equal ? "equal" : "greater"));
			return po;
		}

		enum n_id : t_integer {
			n_id_cat	= 0LL,
			n_id_type	= 1'000'000'000'000'000LL,
			n_id_mod	= 2'000'000'000'000'000LL
		};

		template <n_id C_base_id>
		struct with_id_name {
			// data
			t_index
				m_id;
			t_text
				m_name;

			t_index id() { return m_id - C_base_id; }
		};

		struct data_with_cat_set {
			using t_cat_map = std::vector<bool>;
			using t_cat_set = std::set<t_integer>;
			using t_cat_vec = std::vector<t_integer>;

			// data
			t_cat_map
				m_cat_map;
			t_cat_set
				m_cat_set;
			t_cat_vec
				m_cat_vec;
		};

		template <n_id C_base_id>
		struct with_cat_set : public with_id_name<C_base_id>, public data_with_cat_set {
			using t_from_cat = with_cat_set<n_id_cat>;
			using t_base = with_id_name<C_base_id>;

			with_cat_set(t_integer p_id, t_text p_name) : t_base{p_id, p_name} {
				m_cat_map.push_back(true);
				m_cat_set.insert(0);
			}

			void inner_cat_add(t_integer p_id) {
				if( p_id > std::ssize(m_cat_map) ) {
					m_cat_map.resize(p_id, false);
				}
				m_cat_map[p_id] = true;
				m_cat_set.insert(p_id);
			}

			bool cat_has(const t_from_cat & p_cat) {
				return ( p_cat.m_id <= std::ssize(m_cat_map) ) && m_cat_map[p_cat.m_id];
			}

			void cat_add_from_set(const t_from_cat & p_cat) {
				for( t_integer v_id : p_cat.m_cat_set ) {
					inner_cat_add(v_id);
				}
			}

			void cat_add_indirect(const t_from_cat & p_cat) {
				inner_cat_add(p_cat.m_id);
				cat_add_from_set(p_cat);
			}

			void cat_add(const t_from_cat & p_cat) {
				m_cat_vec.push_back(p_cat.m_id);
				cat_add_indirect(p_cat);
			}
		};

		struct category : public with_cat_set<n_id_cat> {
			using pointer = category *;
			using with_cat_set::with_cat_set;
		};

		struct type : public with_cat_set<n_id_type> {
			using pointer = type *;
			using with_cat_set::with_cat_set;
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