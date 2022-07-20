module;

#include "../src/pre.h"

export module ksi.var.ops;

import <cmath>;
export import ksi.config;

export namespace ksi {

	namespace var {
	
		enum compare_result : t_integer { compare_less = -1, compare_equal = 0, compare_greater = +1 };

		just::output_base & operator << (just::output_base & p_out, compare_result p_cmp) {
			switch( p_cmp ) {
			case compare_less: p_out.write("less"); break;
			case compare_equal: p_out.write("equal"); break;
			default: p_out.write("greater");
			}
			return p_out;
		}

		struct op_compare {
			template <typename T1, typename T2>
			static compare_result cmp_default(T1 p1, T2 p2) {
				return (p1 < p2) ? compare_less : (
					(p1 > p2) ? compare_greater : compare_equal
				);
			}

			// data
			type_pointer	m_type_1, m_type_2;

			// $category#
			compare_result operator () (category::pointer p1, category::pointer p2) const {
				return cmp_default(p1->m_id, p2->m_id);
			}

			// $type#
			compare_result operator () (type_pointer p1, type_pointer p2) const { return cmp_default(p1->m_id, p2->m_id); }
			
			// $bool#
			compare_result operator () (bool p1, bool p2) const { return cmp_default(p1, p2); }
			
			// $int#
			compare_result operator () (t_integer p1, t_integer p2) const { return cmp_default(p1, p2); }

			// $float#
			compare_result operator () (t_floating p1, t_floating p2) const {
				if( std::isnan(p1) ) { return std::isnan(p2) ? compare_equal : compare_less; }
				if( std::isnan(p2) ) { return compare_greater; }
				return cmp_default(p1, p2);
			}
			// $int# <=> $float#
			compare_result operator () (t_integer p1, t_floating p2) const {
				if( std::isnan(p2) ) { return compare_greater; }
				return cmp_default(static_cast<t_floating>(p1), p2);
			}
			// $float# <=> $int#
			compare_result operator () (t_floating p1, t_integer p2) const {
				if( std::isnan(p1) ) { return compare_less; }
				return cmp_default(p1, static_cast<t_floating>(p2) );
			}

			// $text#
			compare_result operator () (compound_text_pointer p1, compound_text_pointer p2) const {
				int v_cmp = just::text_traits::cmp(p1->m_text->m_text, p2->m_text->m_text);
				return cmp_default(v_cmp, 0);
			}

			// $array#
			compare_result operator () (compound_array_pointer p1, compound_array_pointer p2) const {
				compare_result v_cmp = cmp_default(p1->count(), p2->count() );
				if( v_cmp ) { return v_cmp; }
				return cmp_default(p1, p2);
			}

			// $map#
			compare_result operator () (compound_map_pointer p1, compound_map_pointer p2) const {
				compare_result v_cmp = cmp_default(p1->count(), p2->count() );
				if( v_cmp ) { return v_cmp; }
				return cmp_default(p1, p2);
			}

			// type_struct
			compare_result operator () (compound_struct_pointer p1, compound_struct_pointer p2) const {
				compare_result v_cmp = cmp_default(m_type_1->m_id, m_type_2->m_id);
				if( v_cmp ) { return v_cmp; }
				return cmp_default(p1, p2);
			}

			// types vary
			template <typename T1, typename T2>
			compare_result operator () (T1 p1, T2 p2) const { return cmp_default(m_type_1->m_id, m_type_2->m_id); }
		};

		compare_result compare(const any & p1, const any & p2) {
			any_const_pointer v_p1 = p1.any_get_const(), v_p2 = p2.any_get_const();
			op_compare v_cmp{v_p1->m_type, v_p2->m_type};
			t_variant v1, v2;
			v_p1->variant_set(v1);
			v_p2->variant_set(v2);
			return std::visit<compare_result>(v_cmp, v1, v2);
		}

	} // ns

} // ns