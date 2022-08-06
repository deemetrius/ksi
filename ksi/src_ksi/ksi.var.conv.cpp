module ksi.var;

#include "../src/pre.h"

import <cmath>;
import <cstdlib>;
import ksi.config;

namespace ksi { namespace var {

	// to $bool#
	struct vis_conv_bool {
		bool m_bad_conversion = false;
		// $null#
		bool operator () (variant_null) { return false; }
		// $all#
		bool operator () (variant_all) { return true; }
		// $category#
		bool operator () (category::pointer) { return false; }
		// $type#
		bool operator () (type_pointer p_value) { return p_value != &g_config->m_null; }
		// $bool#
		bool operator () (bool p_value) { return p_value; }
		// $int#
		bool operator () (t_integer p_value) { return p_value; }
		// $float#
		bool operator () (t_floating p_value) { return std::isnan(p_value) ? false : (p_value != 0.0); }
		// $text# $array# $map# _struct
		bool operator () (countable::pointer p_value) { return p_value->count(); }
		// other
		template <typename T>
		bool operator () (T) { m_bad_conversion = true; return false; }
	};

	// to $int#
	struct vis_conv_int {
		bool m_bad_conversion = false;
		// $null#
		t_integer operator () (variant_null) { return 0; }
		// $all#
		t_integer operator () (variant_all) { return type_int::t_limits::max(); }
		// $category#
		t_integer operator () (category::pointer p_value) { return p_value->m_id; }
		// $type#
		t_integer operator () (type_pointer p_value) { return p_value->m_id; }
		// $bool#
		t_integer operator () (bool p_value) { return p_value; }
		// $int#
		t_integer operator () (t_integer p_value) { return p_value; }
		// $float#
		t_integer operator () (t_floating p_value) { return std::isnan(p_value) ? 0 : static_cast<t_integer>(p_value); }
		// $text#
		t_integer operator () (compound_text_pointer p_value) { return std::strtoll(p_value->m_text.data(), nullptr, 10); }
		// $array# $map# _struct
		t_integer operator () (countable::pointer p_value) { return p_value->count(); }
		// other
		template <typename T>
		t_integer operator () (T) { m_bad_conversion = true; return 0; }
	};

	// to $float#
	struct vis_conv_float {
		bool m_bad_conversion = false;
		// $null#
		t_floating operator () (variant_null) { return 0.0; }
		// $all#
		t_floating operator () (variant_all) { return type_float::t_limits::infinity(); }
		// $category#
		t_floating operator () (category::pointer p_value) { return static_cast<t_floating>(p_value->m_id); }
		// $type#
		t_floating operator () (type_pointer p_value) { return static_cast<t_floating>(p_value->m_id); }
		// $bool#
		t_floating operator () (bool p_value) { return p_value; }
		// $int#
		t_floating operator () (t_integer p_value) { return static_cast<t_floating>(p_value); }
		// $float#
		t_floating operator () (t_floating p_value) { return p_value; }
		// $text#
		t_floating operator () (compound_text_pointer p_value) { return std::strtod(p_value->m_text.data(), nullptr); }
		// $array# $map# _struct
		t_floating operator () (countable::pointer p_value) { return static_cast<t_floating>(p_value->count() ); }
		// other
		template <typename T>
		t_floating operator () (T) { m_bad_conversion = true; return 0.0; }
	};

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

	void type_bool::from(any_var & p_to, any_var & p_from, bool & p_bad_conversion) {
		any_const_pointer v_from = p_from.any_get_const();
		t_variant v_variant;
		v_from->variant_set(v_variant);
		vis_conv_bool v_visitor;
		p_to = std::visit<bool>(v_visitor, v_variant);
		p_bad_conversion = v_visitor.m_bad_conversion;
	}

	void type_int::from(any_var & p_to, any_var & p_from, bool & p_bad_conversion) {
		any_const_pointer v_from = p_from.any_get_const();
		t_variant v_variant;
		v_from->variant_set(v_variant);
		vis_conv_int v_visitor;
		p_to = std::visit<t_integer>(v_visitor, v_variant);
		p_bad_conversion = v_visitor.m_bad_conversion;
	}

	void type_float::from(any_var & p_to, any_var & p_from, bool & p_bad_conversion) {
		any_const_pointer v_from = p_from.any_get_const();
		t_variant v_variant;
		v_from->variant_set(v_variant);
		vis_conv_float v_visitor;
		p_to = std::visit<t_floating>(v_visitor, v_variant);
		p_bad_conversion = v_visitor.m_bad_conversion;
	}

} } // ns ns