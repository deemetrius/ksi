module;

#include "../src/pre.h"

export module ksi.var:array;

export import :value;

export namespace ksi {

	namespace var {

		struct value_array : public value_base, public marker_array {
			using t_items = optr_nest::template ot_vector<value>;
			using t_items_pointer = t_items *;
			using t_items_ptr = optr_nest::optr<t_items>;

			// data
			t_items_ptr
				m_ptr;

			value_array(junction::pointer p_owner) : m_ptr{p_owner} {
				//just::g_console << "{value_array}\n";
			}
			value_array(junction::pointer p_owner, value_array * p_other) : m_ptr{p_owner, p_other->m_ptr} {
				//just::g_console << "{value_array copy}\n";
			}

			//~value_array() { just::g_console << "{~value_array}\n"; }

			t_type get_type() const override { return &config::handle->mt_array; }
			t_ptr copy(junction::pointer p_owner) override { return std::make_unique<value_array>(p_owner, this); }
			t_variant variant() override { return static_cast<t_array>(this); }

			static t_items_pointer get(value::pointer p_value) {
				return static_cast<value_array *>( p_value->m_value.get() )->m_ptr.get();
			}
			static t_items_pointer get(t_array p_value) {
				return static_cast<value_array *>(p_value)->m_ptr.get();
			}
		};

		value::value(marker_array) : m_value{std::make_unique<value_array>( this->m_point.get() )} {}
		value & value::operator = (marker_array) {
			m_value = std::make_unique<value_array>( this->m_point.get() ); return *this;
		}

	} // ns

} // ns