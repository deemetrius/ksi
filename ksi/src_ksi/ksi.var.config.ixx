module;

#include "../src/pre.h"

export module ksi.var:config;

export import :types;

export namespace ksi {

	using namespace std::string_literals;

	enum n_id : t_integer {
		n_id_cat	= 0x00'00'00'00'00'00'00'00,
		n_id_type	= 0x00'00'01'00'00'00'00'00,
		n_id_mod	= 0x00'00'02'00'00'00'00'00
	};

	struct config {
		using pointer = config *;

		// data
		t_integer
			m_cat_id	= n_id_cat,
			m_type_id	= n_id_type;
		var::category
			mc_any,
			mc_null,
			mc_number;
		var::type
			mt_cat,
			mt_type,
			mt_bool,
			mt_int,
			mt_float,
			mt_text,
			mt_array,
			mt_map;

		config() :
			// cats
			mc_any		{m_cat_id++, L"_any"s},
			mc_null		{m_cat_id++, L"_null"s},
			mc_number	{m_cat_id++, L"_number"s},
			// types
			mt_cat		{m_type_id++, L"$cat"s},
			mt_type		{m_type_id++, L"$type"s},
			mt_bool		{m_type_id++, L"$bool"s},
			mt_int		{m_type_id++, L"$int"s},
			mt_float	{m_type_id++, L"$float"s},
			mt_text		{m_type_id++, L"$text"s},
			mt_array	{m_type_id++, L"$array"s},
			mt_map		{m_type_id++, L"$map"s}
		{
			mt_cat.is_map_key = true;
			mt_type.is_map_key = true;
			mt_bool.is_map_key = true;
			mt_int.is_map_key = true;
			mt_text.is_map_key = true;
		}
	};

	config::pointer hcfg{nullptr};

	void make_config() {
		static config cfg;
		hcfg = &cfg;
	}

	void set_config(config::pointer p) { hcfg = p; }

} // ns