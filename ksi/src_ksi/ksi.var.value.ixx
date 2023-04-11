module;

#include "../src/pre.h"

export module ksi.var:value;

export import :config;
export import just.aux;
export import just.optr;
export import <concepts>;

export namespace ksi {

	namespace var {

		struct optr_nest : public just::optr_nest<optr_nest> {};

		//using ring = just::owned_ring;
		//using optr_nest = just::optr_nest<ring>;
		using junction = var::optr_nest::junction;

		struct value_base : public just::with_deleter<value_base> {
			using pointer = value_base *;
			using t_ptr = std::unique_ptr<value_base, just::hold_deleter>;

			virtual ~value_base() = default;

			virtual t_type get_type() const = 0;
			virtual t_ptr copy(junction::pointer p_owner) = 0;
			virtual void write(just::output_base & o) {}
			virtual t_variant variant() = 0;
		};

		template <typename T>
		struct value_exact : public value_base {
			// data
			T	m_value;

			value_exact(T p_value) : m_value{p_value} {}

			t_type get_type() const override;
			t_ptr copy(junction::pointer p_owner) override { return std::make_unique<value_exact>(*this); }
			void write(just::output_base & o) override {
				if constexpr ( std::is_base_of_v<data_with_cat_set, std::remove_pointer_t<T> > ) {
					o << m_value->m_name->c_str();
				} else if constexpr ( std::is_same_v<t_text, T> ) {
					o << m_value->c_str();
				} else if constexpr ( std::is_same_v<bool, T> ) {
					o << static_cast<t_integer>(m_value);
				} else {
					o << m_value;
				}
			}
			t_variant variant() override { return m_value; }
		};

		//

		using value_cat = value_exact<t_cat>;
		using value_type = value_exact<t_type>;
		using value_bool = value_exact<bool>;
		using value_int = value_exact<t_integer>;
		using value_float = value_exact<t_floating>;
		using value_text = value_exact<t_text>;

		template <typename T> struct vtype;
		template <> struct vtype<t_cat> { using type = value_cat; };
		template <> struct vtype<t_type> { using type = value_type; };
		template <> struct vtype<bool> { using type = value_bool; };
		template <> struct vtype<t_integer> { using type = value_int; };
		template <> struct vtype<t_floating> { using type = value_float; };

		template <> t_type value_cat::get_type() const { return &config::handle->mt_cat; }
		template <> t_type value_type::get_type() const { return &config::handle->mt_type; }
		template <> t_type value_bool::get_type() const { return &config::handle->mt_bool; }
		template <> t_type value_int::get_type() const { return &config::handle->mt_int; }
		template <> t_type value_float::get_type() const { return &config::handle->mt_float; }
		template <> t_type value_text::get_type() const { return &config::handle->mt_text; }

		//

		struct value : public optr_nest::is_target<value> {
			using pointer = value *;

			// data
			value_base::t_ptr
				m_value;

			value() : m_value{std::make_unique<value_cat>(&config::handle->mc_null)} {}

			template <just::c_any_of<t_cat, t_type, bool, t_integer, t_floating> T>
			value(T p) : m_value{std::make_unique< vtype<T>::type >(p)} {}

			template <just::c_any_of<t_cat, t_type, bool, t_integer, t_floating> T>
			value & operator = (T p) { m_value = std::make_unique< vtype<T>::type >(p); return *this; }

			//

			value(const value & p) : m_value{p->copy( this->m_point.get() )} {}
			value(value && p) : m_value{p->copy( this->m_point.get() )} {}

			value & operator = (const value & p) { m_value = p->copy( this->m_point.get() ); return *this; }
			value & operator = (value && p) { m_value = p->copy( this->m_point.get() ); return *this; }

			//

			value(t_text p_value) : m_value{std::make_unique<value_text>(p_value)} {}
			value(marker_array);
			value(marker_map);

			value & operator = (t_text p_value) { m_value = std::make_unique<value_text>(p_value); return *this; }
			value & operator = (marker_array);
			value & operator = (marker_map);

			//

			void reset() { m_value.reset(); }
			value_base::pointer operator -> () const { return m_value.get(); }

			void unset_elements() { m_value.reset(); }
		};

		using cell = optr_nest::optr<value>;

	} // ns

} // ns