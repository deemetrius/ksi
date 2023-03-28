module;

#include "../src/pre.h"

export module ksi.var:ops;

export import :value;
import <cmath>;
import <cwchar>;

export namespace ksi {

	namespace var {
	
		struct t_comparator {
			// data
			t_type
				m_type_1,
				m_type_2;

			// float float
			t_compare operator () (t_floating p1, t_floating p2) const {
				return std::isnan(p1) ? (
					std::isnan(p2) ? t_compare::equal : t_compare::less
				) : (
					std::isnan(p2) ? t_compare::greater : cmp(p1, p2)
				);
			}

			// int float
			t_compare operator () (t_integer p1, t_floating p2) const {
				return std::isnan(p2) ? t_compare::greater : cmp(static_cast<t_floating>(p1), p2);
			}
			// float int
			t_compare operator () (t_floating p1, t_integer p2) const {
				return std::isnan(p1) ? t_compare::less : cmp(p1, static_cast<t_floating>(p2) );
			}

			// text text
			t_compare operator () (t_text p1, t_text p2) const {
				return cmp(
					std::wcscmp(p1.m_ptr->c_str(), p2.m_ptr->c_str() ), 0
				);
			}

			// cat|type same
			template <just::c_any_of<t_cat, t_type> T>
			t_compare operator () (T p1, T p2) const { return cmp(p1->m_id, p2->m_id); }

			// bool|int same
			template <just::c_any_of<bool, t_integer> T>
			t_compare operator () (T p1, T p2) const { return cmp(p1, p2); }

			// array|map same
			template <just::c_any_of<t_array, t_map> T>
			t_compare operator () (T p1, T p2) const { return cmp(p1, p2); }

			// other
			template <typename T1, just::c_not_those<T1> T2>
			t_compare operator () (T1 p1, T2 p2) const {
				return cmp(m_type_1->m_id, m_type_2->m_id);
			}

			// helper
			template <typename T1, typename T2>
			static t_compare cmp(T1 p1, T2 p2) {
				return p1 < p2 ? t_compare::less : (p2 < p1 ? t_compare::greater : t_compare::equal);
			}
		};

		t_compare compare(const value & p1, const value & p2) {
			t_variant
				v1 = p1->variant(),
				v2 = p2->variant();
			t_comparator v_op{p1->get_type(), p2->get_type()};
			return std::visit(v_op, v1, v2);
		}

		struct value_less {
			bool operator () (const value & p1, const value & p2) const {
				return compare(p1, p2) == t_compare::less;
			}

			//static void unset(const value & p) { /*const_cast<value *>(&p)->unset();*/ }
		};

	} // ns

} // ns