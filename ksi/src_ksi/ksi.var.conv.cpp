module ksi.var;

#include "../src/pre.h"

import <cmath>;
import <cstdlib>;
import <cinttypes>;
import ksi.config;

namespace ksi { namespace var {

	using namespace just::text_literals;
	using namespace std::literals::string_view_literals;

	// to $bool#
	struct vis_conv_bool {
		using t_result = bool;

		// data
		t_result m_bad_conversion = false;

		// $null#
		t_result operator () (variant_null) { return false; }
		// $all#
		t_result operator () (variant_all) { return true; }
		// $category#
		t_result operator () (category::pointer) { return false; }
		// $type#
		t_result operator () (type_pointer p_value) { return p_value != &g_config->m_null; }
		// $bool#
		t_result operator () (bool p_value) { return p_value; }
		// $int#
		t_result operator () (t_integer p_value) { return p_value; }
		// $float#
		t_result operator () (t_floating p_value) { return std::isnan(p_value) ? false : (p_value != 0.0); }
		// $text# $array# $map# _struct
		t_result operator () (countable::pointer p_value) { return p_value->count(); }
		// other
		template <typename T>
		t_result operator () (T) { m_bad_conversion = true; return false; }
	};

	// to $int#
	struct vis_conv_int {
		using t_result = t_integer;

		// data
		bool m_bad_conversion = false;

		// $null#
		t_result operator () (variant_null) { return 0; }
		// $all#
		t_result operator () (variant_all) { return type_int::s_max; }
		// $category#
		t_result operator () (category::pointer p_value) { return p_value->m_id; }
		// $type#
		t_result operator () (type_pointer p_value) { return p_value->m_id; }
		// $bool#
		t_result operator () (bool p_value) { return p_value; }
		// $int#
		t_result operator () (t_integer p_value) { return p_value; }
		// $float#
		t_result operator () (t_floating p_value) { return std::isnan(p_value) ? 0 : static_cast<t_result>(p_value); }
		// $text#
		t_result operator () (compound_text_pointer p_value) { return std::strtoimax(p_value->m_text.data(), nullptr, 10); }
		// $array# $map# _struct
		t_result operator () (countable::pointer p_value) { return p_value->count(); }
		// other
		template <typename T>
		t_result operator () (T) { m_bad_conversion = true; return 0; }
	};

	// to $float#
	struct vis_conv_float {
		using t_result = t_floating;

		// data
		bool m_bad_conversion = false;

		// $null#
		t_result operator () (variant_null) { return 0.0; }
		// $all#
		t_result operator () (variant_all) { return type_float::s_infinity; }
		// $category#
		t_result operator () (category::pointer p_value) { return static_cast<t_result>(p_value->m_id); }
		// $type#
		t_result operator () (type_pointer p_value) { return static_cast<t_result>(p_value->m_id); }
		// $bool#
		t_result operator () (bool p_value) { return p_value; }
		// $int#
		t_result operator () (t_integer p_value) { return static_cast<t_result>(p_value); }
		// $float#
		t_result operator () (t_floating p_value) { return p_value; }
		// $text#
		t_result operator () (compound_text_pointer p_value) { return std::strtod(p_value->m_text.data(), nullptr); }
		// $array# $map# _struct
		t_result operator () (countable::pointer p_value) { return static_cast<t_result>(p_value->count() ); }
		// other
		template <typename T>
		t_result operator () (T) { m_bad_conversion = true; return 0.0; }
	};

	// to _simple_number
	struct vis_conv_number {
		using t_result = any_var;

		// data
		bool m_bad_conversion = false;

		// $null#
		t_result operator () (variant_null) { return type_int::s_zero; }
		// $all#
		t_result operator () (variant_all) { return type_float::s_infinity; }
		// $category#
		t_result operator () (category::pointer p_value) { return p_value->m_id; }
		// $type#
		t_result operator () (type_pointer p_value) { return p_value->m_id; }
		// $bool#
		t_result operator () (bool p_value) { return static_cast<t_integer>(p_value); }
		// $int#
		t_result operator () (t_integer p_value) { return p_value; }
		// $float#
		t_result operator () (t_floating p_value) { return p_value; }
		// $text#
		t_result operator () (compound_text_pointer p_value) {
			t_floating ret_float = std::strtod(p_value->m_text.data(), nullptr);
			if( std::isnan(ret_float) || std::isinf(ret_float) ) { return ret_float; }
			//
			t_text_value::pointer v_end;
			errno = 0;
			t_integer ret_int = std::strtoimax(p_value->m_text.data(), &v_end, 10);
			if( errno == ERANGE || (*v_end == '.' && std::isdigit(v_end[1]) ) || *v_end == 'e' ) { return ret_float; }
			return ret_int;
		}
		// $array# $map# _struct
		t_result operator () (countable::pointer p_value) { return static_cast<t_integer>(p_value->count() ); }
		// other
		template <typename T>
		t_result operator () (T) { m_bad_conversion = true; return type_int::s_zero; }
	};

	// to $text#
	struct vis_conv_text {
		using t_result = t_text_value;

		// data
		type_pointer	m_type;
		bool			m_bad_conversion = false;

		// $null#
		t_result operator () (variant_null) { return ""_jt; }
		// $all#
		t_result operator () (variant_all) { return ""_jt; }
		// $category#
		t_result operator () (category::pointer p_value) { return p_value->m_name_full; }
		// $type#
		t_result operator () (type_pointer p_value) { return p_value->m_name_full; }
		// $bool#
		t_result operator () (bool p_value) { return p_value ? "0"_jt : "1"_jt; }
		// $int#
		t_result operator () (t_integer p_value) {
			int v_len = snprintf(nullptr, 0, "%lld", p_value);
			if( v_len < 0 ) {
				m_bad_conversion = true;
				return "?"_jt;
			}
			int v_size = v_len +1;
			typename t_result::pointer v_text;
			t_result ret{v_size, v_text};
			v_len = snprintf(v_text, v_size, "%lld", p_value);
			if( v_len < 0 ) {
				m_bad_conversion = true;
				return "?"_jt;
			}
			return ret;
		}
		// $float#
		t_result operator () (t_floating p_value) {
			if( std::isnan(p_value) ) { return "NaN"_jt; }
			if( p_value == type_float::s_infinity ) { return "infinity"_jt; }
			if( p_value == -type_float::s_infinity ) { return "-infinity"_jt; }
			int v_len = snprintf(nullptr, 0, "%g", p_value);
			if( v_len < 0 ) {
				m_bad_conversion = true;
				return "?"_jt;
			}
			int v_size = v_len +1;
			typename t_result::pointer v_text;
			t_result ret{v_size, v_text};
			v_len = snprintf(v_text, v_size, "%g", p_value);
			if( v_len < 0 ) {
				m_bad_conversion = true;
				return "?"_jt;
			}
			return ret;
		}
		// $text#
		t_result operator () (compound_text_pointer p_value) { return p_value->m_text; }
		// $array#
		t_result operator () (compound_array_pointer p_value) { return "array"_jt; }
		// $map#
		t_result operator () (compound_map_pointer p_value) { return "map"_jt; }
		// _struct
		t_result operator () (compound_struct_pointer p_value) { return just::implode<t_char>({"struct "sv, m_type->m_name_full}); }
		// other
		template <typename T>
		t_result operator () (T) { m_bad_conversion = true; return ""_jt; }
	};

	// to $array#
	struct vis_conv_array {
		using t_result = any_var;

		// data
		type_pointer		m_type;
		var_const_pointer	m_from;
		bool				m_bad_conversion = false;

		// $null#
		t_result operator () (variant_null) { return any_var{case_array{}, 0}; }
		// $all#
		t_result operator () (variant_all) { return any_var{case_array{}, 0}; }
		// $array#
		t_result operator () (compound_array_pointer p_value) { return *m_from; }
		// $map#
		t_result operator () (compound_map_pointer p_value) {
			t_index v_count = p_value->count_impl();
			compound_array_pointer v_compound;
			any_var ret{case_array{}, v_compound, v_count};
			for( t_index v_index = 0; typename compound_map::t_items::t_node::pointer v_it : p_value->m_items ) {
				v_compound->m_items[v_index] = v_it->m_value;
				++v_index;
			}
			return ret;
		}
		// _struct
		t_result operator () (compound_struct_pointer p_value) {
			t_index v_count = p_value->count_impl();
			compound_array_pointer v_compound;
			any_var ret{case_array{}, v_compound, v_count};
			for( t_index v_index = 0; v_index <= v_count; ++v_index ) {
				v_compound->m_items[v_index] = p_value->m_items[v_index];
			}
			return ret;
		}
		// other
		template <typename T>
		t_result operator () (T) {
			compound_array_pointer v_compound;
			any_var ret{case_array{}, v_compound, 1};
			v_compound->m_items.first() = *m_from;
			return ret;
		}
	};

	// to $map#
	struct vis_conv_map {
		using t_result = any_var;

		// data
		type_pointer		m_type;
		var_const_pointer	m_from;
		bool				m_bad_conversion = false;

		// $null#
		t_result operator () (variant_null) { return any_var{case_array{}, 0}; }
		// $all#
		t_result operator () (variant_all) { return any_var{case_array{}, 0}; }
		// $array#
		t_result operator () (compound_array_pointer p_value) {
			compound_map_pointer v_compound;
			any_var ret{case_map{}, v_compound};
			for( any_var & v_it : p_value->m_items ) {
				v_compound->assign(variant_null{}, v_it);
			}
			return ret;
		}
		// $map#
		t_result operator () (compound_map_pointer p_value) { return *m_from; }
		// _struct
		t_result operator () (compound_struct_pointer p_value) {
			t_index v_count = p_value->count_impl();
			compound_map_pointer v_compound;
			any_var ret{case_map{}, v_compound};
			for( t_index v_index = 0; v_index <= v_count; ++v_index ) {
				v_compound->assign(p_value->m_type->prop_name(v_index), p_value->m_items[v_index]);
			}
			return ret;
		}
		// other
		template <typename T>
		t_result operator () (T) {
			compound_map_pointer v_compound;
			any_var ret{case_map{}, v_compound};
			v_compound->assign(variant_null{}, *m_from);
			return ret;
		}
	};

	// to _struct
	struct vis_conv_struct {
		using t_result = any_var;

		// data
		type_struct_pointer		m_type_target;
		type_pointer			m_type;
		var_const_pointer		m_from;
		bool					m_bad_conversion = false;

		// $array#
		t_result operator () (compound_array_pointer p_value) {
			compound_struct_pointer v_compound;
			any_var ret{m_type_target, v_compound};
			t_index v_count = std::min(p_value->count_impl(), v_compound->count_impl() );
			for( t_index v_index = 0; v_index < v_count; ++v_index ) {
				v_compound->m_items[v_index] = p_value->m_items[v_index];
			}
			return ret;
		}
		// $map#
		t_result operator () (compound_map_pointer p_value) {
			compound_struct_pointer v_compound;
			any_var ret{m_type_target, v_compound};
			t_index v_count = v_compound->count_impl();
			bool v_wrong_key;
			for( t_index v_index = 0; v_index < v_count; ++v_index ) {
				var_pointer v_element = m_from->element(m_type_target->prop_name(v_index), v_wrong_key);
				if( !v_wrong_key ) {
					v_compound->m_items[v_index] = *v_element;
					continue;
				}
				v_element = m_from->element(static_cast<t_integer>(v_index), v_wrong_key);
				if( !v_wrong_key ) {
					v_compound->m_items[v_index] = *v_element;
				}
			}
			return ret;
		}
		// _struct
		t_result operator () (compound_struct_pointer p_value) {
			compound_struct_pointer v_compound;
			any_var ret{m_type_target, v_compound};
			t_index v_count = v_compound->count_impl();
			bool v_wrong_key;
			for( t_index v_index = 0; v_index < v_count; ++v_index ) {
				var_pointer v_element = m_from->element(m_type_target->prop_name(v_index), v_wrong_key);
				if( !v_wrong_key ) {
					v_compound->m_items[v_index] = *v_element;
					continue;
				}
				v_element = m_from->element(static_cast<t_integer>(v_index), v_wrong_key);
				if( !v_wrong_key ) {
					v_compound->m_items[v_index] = *v_element;
				}
			}
			return ret;
		}
		// other
		template <typename T>
		t_result operator () (T) {
			return any_var{m_type_target};
		}
	};

	//

	void type_base::from(any_var & p_to, any_var & p_from, bool & p_bad_conversion) {
		p_to.close();
		p_bad_conversion = true;
	}

	void type_null::from(any_var & p_to, any_var & p_from, bool & p_bad_conversion) {
		p_to.close();
		p_bad_conversion = false;
	}

	void type_all::from(any_var & p_to, any_var & p_from, bool & p_bad_conversion) {
		p_to = variant_all{};
		p_bad_conversion = false;
	}

	type_pointer type_type::from(any_var & p_from, bool & p_bad_conversion) {
		p_bad_conversion = false;
		any_const_pointer v_from = p_from.any_get_const();
		return (v_from->m_type == &g_config->m_type) ? v_from->m_value.m_type : v_from->m_type;
	}

	void type_type::from(any_var & p_to, any_var & p_from, bool & p_bad_conversion) {
		p_bad_conversion = false;
		any_const_pointer v_from = p_from.any_get_const();
		if( v_from->m_type == &g_config->m_type ) {
			p_to = p_from;
			return;
		}
		p_to = v_from->m_type;
	}

	bool type_bool::from(any_var & p_from, bool & p_bad_conversion) {
		any_const_pointer v_from = p_from.any_get_const();
		t_variant v_variant;
		v_from->variant_set(v_variant);
		vis_conv_bool v_visitor;
		bool ret = std::visit<bool>(v_visitor, v_variant);
		p_bad_conversion = v_visitor.m_bad_conversion;
		return ret;
	}

	void type_bool::from(any_var & p_to, any_var & p_from, bool & p_bad_conversion) {
		p_to = from(p_from, p_bad_conversion);
	}

	t_integer type_int::from(any_var & p_from, bool & p_bad_conversion) {
		any_const_pointer v_from = p_from.any_get_const();
		t_variant v_variant;
		v_from->variant_set(v_variant);
		vis_conv_int v_visitor;
		t_integer ret = std::visit<t_integer>(v_visitor, v_variant);
		p_bad_conversion = v_visitor.m_bad_conversion;
		return ret;
	}

	void type_int::from(any_var & p_to, any_var & p_from, bool & p_bad_conversion) {
		p_to = from(p_from, p_bad_conversion);
	}

	t_floating type_float::from(any_var & p_from, bool & p_bad_conversion) {
		any_const_pointer v_from = p_from.any_get_const();
		t_variant v_variant;
		v_from->variant_set(v_variant);
		vis_conv_float v_visitor;
		t_floating ret = std::visit<t_floating>(v_visitor, v_variant);
		p_bad_conversion = v_visitor.m_bad_conversion;
		return ret;
	}

	void type_float::from(any_var & p_to, any_var & p_from, bool & p_bad_conversion) {
		p_to = from(p_from, p_bad_conversion);
	}

	void type_simple_number::number(any_var & p_to, any_var & p_from, bool & p_bad_conversion) {
		any_const_pointer v_from = p_from.any_get_const();
		t_variant v_variant;
		v_from->variant_set(v_variant);
		vis_conv_number v_visitor;
		p_to = std::visit<any_var>(v_visitor, v_variant);
		p_bad_conversion = v_visitor.m_bad_conversion;
	}

	t_text_value type_text::from(any_var & p_from, bool & p_bad_conversion) {
		any_const_pointer v_from = p_from.any_get_const();
		t_variant v_variant;
		v_from->variant_set(v_variant);
		vis_conv_text v_visitor{v_from->m_type};
		t_text_value ret = std::visit<t_text_value>(v_visitor, v_variant);
		p_bad_conversion = v_visitor.m_bad_conversion;
		return ret;
	}

	void type_text::from(any_var & p_to, any_var & p_from, bool & p_bad_conversion) {
		p_to = from(p_from, p_bad_conversion);
	}

	void type_array::from(any_var & p_to, any_var & p_from, bool & p_bad_conversion) {
		any_const_pointer v_from = p_from.any_get_const();
		t_variant v_variant;
		v_from->variant_set(v_variant);
		vis_conv_array v_visitor{v_from->m_type, &p_from};
		p_to = std::visit<any_var>(v_visitor, v_variant);
		p_bad_conversion = v_visitor.m_bad_conversion;
	}

	void type_map::from(any_var & p_to, any_var & p_from, bool & p_bad_conversion) {
		any_const_pointer v_from = p_from.any_get_const();
		t_variant v_variant;
		v_from->variant_set(v_variant);
		vis_conv_map v_visitor{v_from->m_type, &p_from};
		p_to = std::visit<any_var>(v_visitor, v_variant);
		p_bad_conversion = v_visitor.m_bad_conversion;
	}

	void type_struct::from(any_var & p_to, any_var & p_from, bool & p_bad_conversion) {
		any_const_pointer v_from = p_from.any_get_const();
		t_variant v_variant;
		v_from->variant_set(v_variant);
		vis_conv_struct v_visitor{this, v_from->m_type, &p_from};
		p_to = std::visit<any_var>(v_visitor, v_variant);
		p_bad_conversion = v_visitor.m_bad_conversion;
	}

} } // ns ns