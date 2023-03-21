module;

#include "../src/pre.h"

export module ksi.var:array;

export import :value;

export namespace ksi {

	namespace var {

		struct value_array : public value_base, public marker_array {
			using t_items = with_ring::template ot_vector<value>;
			using t_items_pointer = t_items *;
			using t_items_ptr = with_ring::optr<t_items>;

			// data
			t_items_ptr
				m_ptr;

			value_array(owner_pointer p_owner) : m_ptr{p_owner} {}
			value_array(owner_pointer p_owner, value_array * p_other) : m_ptr{p_owner, p_other->m_ptr} {}

			t_type get_type() const override { return &hcfg->mt_array; }
			t_ptr copy(owner_pointer p_owner) override { return std::make_unique<value_array>(p_owner, this); }
			t_variant variant() override { return static_cast<t_array>(this); }

			static t_items_pointer get(value::pointer p_value) {
				return static_cast<value_array *>( p_value->m_value.get() )->m_ptr.get();
			}
			static t_items_pointer get(t_array p_value) {
				return static_cast<value_array *>(p_value)->m_ptr.get();
			}
		};

		value::value(marker_array) : m_value{std::make_unique<value_array>(&this->m_owner)} {}
		value & value::operator = (marker_array) { m_value = std::make_unique<value_array>(&this->m_owner); return *this; }

	} // ns

} // ns