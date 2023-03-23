module;

#include "../src/pre.h"

export module ksi.var:map;

export import :ops;

export namespace ksi {

	namespace var {

		struct value_map : public value_base, public marker_map {
			using t_items = with_ring::template ot_map<value, value, value_less>;
			using t_items_pointer = t_items *;
			using t_items_ptr = with_ring::optr<t_items>;

			// data
			t_items_ptr
				m_ptr;

			value_map(owner_pointer p_owner) : m_ptr{p_owner} {}
			value_map(owner_pointer p_owner, value_map * p_other) : m_ptr{p_owner, p_other->m_ptr} {}

			t_type get_type() const override { return &config::handle->mt_map; }
			t_ptr copy(owner_pointer p_owner) override { return std::make_unique<value_map>(p_owner, this); }
			t_variant variant() override { return static_cast<t_map>(this); }

			static t_items_pointer get(value::pointer p_value) {
				return static_cast<value_map *>( p_value->m_value.get() )->m_ptr.get();
			}
			static t_items_pointer get(t_map p_value) {
				return static_cast<value_map *>(p_value)->m_ptr.get();
			}
		};
	
		value::value(marker_map) : m_value{std::make_unique<value_map>(&this->m_owner)} {}
		value & value::operator = (marker_map) { m_value = std::make_unique<value_map>(&this->m_owner); return *this; }

	} // ns

} // ns