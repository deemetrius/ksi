module;

#include "../src/pre.h"

export module ksi.undefined_yet;

export import ksi.act;

export namespace ksi {

	enum class property_status { n_undefined, n_calculating, n_ready };

	namespace ast {

		struct ext_property {
			// data
			t_text
				m_name;
			act::sequence
				m_seq;
		};

	} // ns

	struct t_property {
		// data
		var::cell
			m_cell;
		t_index
			m_seq_index;
		property_status
			m_status = property_status::n_undefined;

		t_property(var::optr_nest::junction::pointer p_point, t_index p_seq_index) :
			m_cell{p_point},
			m_seq_index{p_seq_index}
		{}
	};

} // ns