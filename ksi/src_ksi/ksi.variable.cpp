module ksi.var;

#include "../src/pre.h"

import ksi.config;
import ksi.var.ops;

namespace ksi {

	namespace var {

		void type_base::name(const t_text_value & p_name) {
			m_name = p_name;
			m_name_full = just::implode<t_text_value::type>({p_name, m_module->name()});
		}

		// $null#
		any::any() : m_type{&g_config->m_null} {}

		// $null#
		any::any(variant_null) : m_type{&g_config->m_null} {}

		// $all#
		any::any(variant_all) : m_type{&g_config->m_all} {}

		// $bool#
		any::any(bool p_value) : m_type{&g_config->m_bool} {
			m_value.m_bool = p_value;
		}

		// $int#
		any::any(t_integer p_value) : m_type{&g_config->m_int} {
			m_value.m_int = p_value;
		}

		// $float#
		any::any(t_floating p_value) : m_type{&g_config->m_float} {
			m_value.m_float = p_value;
		}

		// $type#
		any::any(type_pointer p_value) : m_type{&g_config->m_type} {
			m_value.m_type = p_value;
		}

		// copy
		any_var::any_var(const any_var & p_other) : any() {
			if( &p_other == &g_config->m_zero_var ) { return; }
			any_const_pointer v_from = p_other.any_get_const();
			v_from->m_type->var_change(this, v_from);
		}

		// move
		any_var::any_var(any_var && p_other) : any() {
			if( &p_other == &g_config->m_zero_var ) { return; }
			p_other.m_type->var_change(this, &p_other);
		}

		// copy assign
		any_var & any_var::operator = (const any_var & p_other) {
			if( this == &g_config->m_zero_var ) { return *this; }
			any_const_pointer v_from = p_other.any_get_const();
			v_from->m_type->var_change(this, v_from);
			return *this;
		}

		// move assign
		any_var & any_var::operator = (any_var && p_other) {
			if( this == &g_config->m_zero_var ) { return *this; }
			p_other.m_type->var_change(this, &p_other);
			return *this;
		}

		// $text#
		any_var::any_var(const t_text_value & p_text) : any{} {
			link_pointer v_link = link_make_maybe();
			v_link->m_type = &g_config->m_text;
			compound_text_pointer v_compound_text;
			v_link->m_value.m_compound = v_compound_text = new compound_text(p_text);
			v_compound_text->link_text(v_link);
		}

		// struct
		any_var::any_var(type_struct_pointer p_type) : any{} {
			link_pointer v_link = link_make_maybe();
			v_link->m_type = p_type;
			compound_struct_pointer v_compound;
			v_link->m_value.m_compound = v_compound = new compound_struct(p_type);
			v_compound->link(v_link);
		}

		void any_var::link_to(var_pointer p_var) {
			p_var->link_make_maybe();
			g_config->m_ref.var_change(this, p_var);
		}

		void any_var::ref_to(var_pointer p_var) {
			link_to(p_var);
			m_type = &g_config->m_ref;
		}

		//

		static_data::static_data(const t_text_value & p_type_name, t_integer p_id) :
			m_id_props{p_id + n_id_delta_static_props},
			m_id_consts{p_id + n_id_delta_static_consts},
			m_struct_props{just::implode<t_text_value::type>({p_type_name, ".$static_props"}),
				&g_config->m_mod_hidden, m_id_props, true
			},
			m_struct_consts{just::implode<t_text_value::type>({p_type_name, ".$static_consts#"}),
				&g_config->m_mod_hidden, m_id_consts, true
			}
		{}

		//

		bool any_less::operator () (const any & p1, const any & p2) const {
			return compare(p1, p2) == compare_less;
		}

		// var_owner()

		compound_pointer type_link::var_owner(var_const_pointer p_var) {
			return p_var->m_owner_node->m_owner;
		}

		compound_pointer type_simple::var_owner(var_const_pointer p_var) {
			return p_var->m_owner;
		}

		compound_pointer type_compound::var_owner(var_const_pointer p_var) {
			using namespace just::text_literals;
			g_config->m_log->add({g_config->m_path,
				"system bad case: var_owner() was called for compound type."_jt
				});
			return p_var->m_owner;
		}

		// var_owner_set()

		void type_link::var_owner_set(var_pointer p_var, compound_pointer p_owner) {
			p_var->m_owner_node->m_owner = p_owner;
		}

		void type_simple::var_owner_set(var_pointer p_var, compound_pointer p_owner) {
			p_var->m_owner = p_owner;
		}

		void type_compound::var_owner_set(var_pointer p_var, compound_pointer p_owner) {
			using namespace just::text_literals;
			g_config->m_log->add({g_config->m_path,
				"system bad case: var_owner_set() was called for compound type."_jt
				});
			p_var->m_owner = p_owner;
		}

		// link_make_maybe()

		link_pointer type_link::link_make_maybe(var_pointer p_var) {
			return p_var->m_value.m_link;
		}

		link_pointer type_simple::link_make_maybe(var_pointer p_var) {
			// make link
			link_pointer v_link = new any_link;
			v_link->m_value = p_var->m_value;
			v_link->m_type = p_var->m_type;
			// update var
			p_var->m_value.m_link = v_link;
			p_var->m_type = &g_config->m_link;
			// owner_node node
			p_var->m_owner_node = new owner_node{p_var->m_owner};
			v_link->m_owners.node_attach(p_var->m_owner_node);
			return v_link;
		}

		link_pointer type_compound::link_make_maybe(var_pointer p_var) {
			using namespace just::text_literals;
			g_config->m_log->add({g_config->m_path,
				"system bad case: link_make_maybe() was called for compound type."_jt
				});
			return g_config->m_null.link_make_maybe(p_var);
		}

		// any_get()

		any_pointer type_link::any_get(any_pointer p_any) {
			return p_any->m_value.m_link;
		}

		any_pointer type_simple::any_get(any_pointer p_any) {
			return p_any;
		}

		any_pointer type_compound::any_get(any_pointer p_any) {
			return p_any;
		}

		// any_get_const()

		any_const_pointer type_link::any_get_const(any_const_pointer p_any) {
			return p_any->m_value.m_link;
		}

		any_const_pointer type_simple::any_get_const(any_const_pointer p_any) {
			return p_any;
		}

		any_const_pointer type_compound::any_get_const(any_const_pointer p_any) {
			return p_any;
		}

		// any_close()

		void type_link::any_close(any_pointer p_any) {
			link_pointer v_link = p_any->m_value.m_link;
			var_pointer v_var = static_cast<var_pointer>(p_any);
			owner_node_pointer v_owner_node = v_var->m_owner_node;
			v_var->m_owner = v_owner_node->m_owner;
			p_any->m_type = &g_config->m_null;
			v_owner_node->node_detach();
			if( v_link->m_owners.node_empty() ) { // no other links
				v_link->m_deleter(v_link);
			} else { // some other links
				if(
					v_link->m_type->m_is_compound &&
					v_owner_node->m_owner != v_link->link_owner()
					) {
					// rebind compound
					any_var v_tmp;
					v_link->m_type->var_change(&v_tmp, v_link);
					type_compound::link_change(v_link, v_tmp.m_value.m_link);
				}
			}
			v_owner_node->m_deleter(v_owner_node);
		}

		void type_simple::any_close(any_pointer p_any) {
			p_any->m_type = &g_config->m_null;
		}

		void type_compound::any_close(any_pointer p_any) {
			link_pointer v_link = static_cast<link_pointer>(p_any);
			compound_pointer v_compound = v_link->m_value.m_compound;
			if( v_compound->link_is_primary(v_link) ) { // primary link
				v_link->node_detach();
				if( v_compound->m_links_strong.node_empty() ) { // no strong links
																// reset weak links
					v_compound->m_links_weak.node_apply_to_others(
						[](link_node_pointer p_node){
							p_node->node_target()->close();
						}
					);
					// delete compound
					v_compound->m_deleter(v_compound);
				}
			} else { // not primary link
				v_link->node_detach();
			}
			p_any->m_type = &g_config->m_null;
		}

		// var_change()

		void type_link::var_change(var_pointer p_to, any_const_pointer p_from) {
			link_pointer v_link = p_from->m_value.m_link;
			v_link->m_type->var_change(p_to, v_link);
		}

		void type_ref::var_change(var_pointer p_to, any_const_pointer p_from) {
			p_to->close();
			link_pointer v_link = p_from->m_value.m_link;
			// update var
			p_to->m_value.m_link = v_link;
			p_to->m_type = &g_config->m_link;
			// owner_node
			p_to->m_owner_node = new owner_node{p_to->m_owner};
			v_link->m_owners.m_prev->node_attach(p_to->m_owner_node);
			// close zero var
			g_config->m_zero_var.close();
		}

		void type_simple::var_change(var_pointer p_to, any_const_pointer p_from) {
			any_pointer v_any = p_to->any_get();
			v_any->close();
			v_any->m_value = p_from->m_value;
			v_any->m_type = p_from->m_type;
		}

		void type_compound::var_change(var_pointer p_to, any_const_pointer p_from) {
			link_pointer v_link = p_to->link_make_maybe();
			link_change(v_link, p_from);
		}

		void type_compound::link_change(link_pointer p_link, any_const_pointer p_from) {
			p_link->close();
			compound_pointer v_compound = p_from->m_value.m_compound;
			p_link->m_value.m_compound = v_compound;
			v_compound->link(p_link);
			p_link->m_type = p_from->m_type;
		}

		// write()

		bool type_link::write(
			output_pointer p_out,
			any_const_pointer p_any,
			const any & p_separator,
			set_deep & p_deep
		) {
			return p_any->m_value.m_link->write(p_out);
		}

		bool type_type::write(
			output_pointer p_out,
			any_const_pointer p_any,
			const any & p_separator,
			set_deep & p_deep
		) {
			return p_out->write(p_any->m_value.m_type->m_name_full.data() );
		}

		bool type_bool::write(
			output_pointer p_out,
			any_const_pointer p_any,
			const any & p_separator,
			set_deep & p_deep
		) {
			return p_out->write(p_any->m_value.m_bool ? 1 : 0);
		}

		bool type_int::write(
			output_pointer p_out,
			any_const_pointer p_any,
			const any & p_separator,
			set_deep & p_deep
		) {
			return p_out->write(p_any->m_value.m_int);
		}

		bool type_float::write(
			output_pointer p_out,
			any_const_pointer p_any,
			const any & p_separator,
			set_deep & p_deep
		) {
			return p_out->write(p_any->m_value.m_float);
		}

		bool type_text::write(
			output_pointer p_out,
			any_const_pointer p_any,
			const any & p_separator,
			set_deep & p_deep
		) {
			return p_out->write(p_any->m_value.m_compound->get_text()->m_text->m_text);
		}

		bool type_array::write(
			output_pointer p_out,
			any_const_pointer p_any,
			const any & p_separator,
			set_deep & p_deep
		) {
			compound_pointer v_compound = p_any->m_value.m_compound;
			if( p_deep.contains(p_any->m_value.m_compound) ) {
				return p_out->write("{recursion}");
			}
			set_deep_changer v_deep_c{p_deep, v_compound};
			compound_array_pointer v_array = v_compound->get_array();
			t_int_ptr v_errors = 0;
			any_const_pointer v_separator = p_separator.any_get_const();
			if( v_separator->m_type == &g_config->m_null ) {
				for( any_var & v_it : v_array->m_items ) {
					v_errors += v_it.write(p_out, p_separator, p_deep);
				}
				return v_errors;
			}
			if( v_separator->m_type == &g_config->m_array ) {
				bool v_wrong_key;
				var_pointer v_sep_items = v_separator->element(t_integer{0}, v_wrong_key);
				var_pointer v_sep_keys = v_separator->element(t_integer{1}, v_wrong_key);
				bool is_first = true;
				for( t_index v_it = 0, v_count = v_array->count(); v_it < v_count; ++v_it ) {
					if( is_first ) { is_first = false; }
					else { v_errors += v_sep_items->write(p_out); }
					v_errors += p_out->write(v_it);
					v_errors += v_sep_keys->write(p_out);
					v_errors += v_array->m_items[v_it].write(p_out, p_separator, p_deep);
				}
				return v_errors;
			}
			bool is_first = true;
			for( any_var & v_it : v_array->m_items ) {
				if( is_first ) { is_first = false; }
				else { v_errors += v_separator->write(p_out); }
				v_errors += v_it.write(p_out, p_separator, p_deep);
			}
			return v_errors;
		}

		bool type_map::write(
			output_pointer p_out,
			any_const_pointer p_any,
			const any & p_separator,
			set_deep & p_deep
		) {
			compound_pointer v_compound = p_any->m_value.m_compound;
			if( p_deep.contains(p_any->m_value.m_compound) ) {
				return p_out->write("{recursion}");
			}
			set_deep_changer v_deep_c{p_deep, v_compound};
			compound_map_pointer v_map = v_compound->get_map();
			t_int_ptr v_errors = 0;
			any_const_pointer v_separator = p_separator.any_get_const();
			if( v_separator->m_type == &g_config->m_null ) {
				for( typename compound_map::t_items::t_node::pointer v_it : v_map->m_items ) {
					v_errors += v_it->m_value.write(p_out, p_separator, p_deep);
				}
				return v_errors;
			}
			if( v_separator->m_type == &g_config->m_array ) {
				bool v_wrong_key;
				var_pointer v_sep_items = v_separator->element(t_integer{0}, v_wrong_key);
				var_pointer v_sep_keys = v_separator->element(t_integer{1}, v_wrong_key);
				bool is_first = true;
				for( typename compound_map::t_items::t_node::pointer v_it : v_map->m_items ) {
					if( is_first ) { is_first = false; }
					else { v_errors += v_sep_items->write(p_out); }
					v_errors += (*v_it->m_key_iterator).first.write(p_out);
					v_errors += v_sep_keys->write(p_out);
					v_errors += v_it->m_value.write(p_out, p_separator, p_deep);
				}
				return v_errors;
			}
			bool is_first = true;
			for( typename compound_map::t_items::t_node::pointer v_it : v_map->m_items ) {
				if( is_first ) { is_first = false; }
				else { v_errors += v_separator->write(p_out); }
				v_errors += v_it->m_value.write(p_out, p_separator, p_deep);
			}
			return v_errors;
		}

		// element()

		var_pointer type_base::element(any_const_pointer p_any, any_const_pointer p_key, bool & p_wrong_key) {
			p_wrong_key = true;
			return &g_config->m_zero_var;
		}

		var_pointer type_type::element(any_const_pointer p_any, any_const_pointer p_key, bool & p_wrong_key) {
			static_data_pointer v_static = p_any->m_type->get_static();
			any_pointer v_any = v_static->m_props.any_get();
			return v_any->element(p_key, p_wrong_key);
		}

		var_pointer type_array::element(any_const_pointer p_any, any_const_pointer p_key, bool & p_wrong_key) {
			compound_array_pointer v_array = p_any->m_value.m_compound->get_array();
			if( t_integer v_count = v_array->count() ) {
				if( p_key->m_type == &g_config->m_int ) { // int key
					t_integer v_key = p_key->m_value.m_int;
					if( v_key >= 0 && v_key < v_count ) {
						p_wrong_key = false;
						return v_array->m_items.data() + v_key;
					}
				} else if( p_key->m_type == &g_config->m_text ) { // text key
					t_text_value_pointer v_key = &p_key->m_value.m_compound->get_text()->m_text;
					if( just::text_traits::cmp((*v_key)->m_text, "last") == 0 ) { // last
						p_wrong_key = false;
						return v_array->m_items.data() + v_count -1;
					}
				}
			}
			p_wrong_key = true;
			return &g_config->m_zero_var;
		}

		var_pointer type_map::element(any_const_pointer p_any, any_const_pointer p_key, bool & p_wrong_key) {
			compound_map_pointer v_map = p_any->m_value.m_compound->get_map();
			if( v_map->count() ) {
				if( typename compound_map::t_items::t_node::pointer v_node = v_map->m_items.find(p_key) ) {
					p_wrong_key = false;
					return &v_node->m_value;
				}
			}
			p_wrong_key = true;
			return &g_config->m_zero_var;
		}

		var_pointer type_struct::element(any_const_pointer p_any, any_const_pointer p_key, bool & p_wrong_key) {
			if( m_props.count() ) {
				compound_struct_pointer v_compound = p_any->m_value.m_compound->get_struct();
				if( p_key->m_type == &g_config->m_int ) { // int key
					t_integer v_key = p_key->m_value.m_int;
					if( v_key >= 0 && v_key < v_compound->count() ) {
						p_wrong_key = false;
						return v_compound->m_items.data() + v_key;
					}
				} else if( p_key->m_type == &g_config->m_text ) { // text key
					t_text_value_pointer v_key = &p_key->m_value.m_compound->get_text()->m_text;
					typename t_props::t_find_result v_res = m_props.find(*v_key);
					if( v_res.m_added ) {
						p_wrong_key = false;
						return v_compound->m_items.data() + v_res.m_index;
					}
				}
			}
			p_wrong_key = true;
			return &g_config->m_zero_var;
		}

		// element_const()

		var_pointer type_base::element_const(any_const_pointer p_any, const t_text_value & p_key, bool & p_wrong_key) {
			p_wrong_key = true;
			return &g_config->m_zero_var;
		}

		var_pointer type_type::element_const(any_const_pointer p_any, const t_text_value & p_key, bool & p_wrong_key) {
			static_data_pointer v_static = p_any->m_type->get_static();
			any_pointer v_any = v_static->m_consts.any_get();
			any_var v_key{p_key};
			return v_any->element(v_key, p_wrong_key);
		}

		var_pointer type_text::element_const(any_const_pointer p_any, const t_text_value & p_key, bool & p_wrong_key) {
			compound_text_pointer v_compound = p_any->m_value.m_compound->get_text();
			if( just::text_traits::cmp(p_key->m_text, "count#") == 0 ) { // count
				v_compound->m_count = t_integer{v_compound->count()};
				p_wrong_key = false;
				return &v_compound->m_count;
			}
			p_wrong_key = true;
			return &g_config->m_zero_var;
		}

		var_pointer type_array::element_const(any_const_pointer p_any, const t_text_value & p_key, bool & p_wrong_key) {
			compound_array_pointer v_compound = p_any->m_value.m_compound->get_array();
			if( just::text_traits::cmp(p_key->m_text, "count#") == 0 ) { // count
				v_compound->m_count = v_compound->count();
				p_wrong_key = false;
				return &v_compound->m_count;
			}
			p_wrong_key = true;
			return &g_config->m_zero_var;
		}

		var_pointer type_map::element_const(any_const_pointer p_any, const t_text_value & p_key, bool & p_wrong_key) {
			compound_map_pointer v_compound = p_any->m_value.m_compound->get_map();
			if( just::text_traits::cmp(p_key->m_text, "count#") == 0 ) { // count
				v_compound->m_count = t_integer{v_compound->count()};
				p_wrong_key = false;
				return &v_compound->m_count;
			}
			p_wrong_key = true;
			return &g_config->m_zero_var;
		}

		var_pointer type_struct::element_const(any_const_pointer p_any, const t_text_value & p_key, bool & p_wrong_key) {
			compound_struct_pointer v_compound = p_any->m_value.m_compound->get_struct();
			if( just::text_traits::cmp(p_key->m_text, "count#") == 0 ) { // count
				v_compound->m_count = t_integer{v_compound->count()};
				p_wrong_key = false;
				return &v_compound->m_count;
			}
			p_wrong_key = true;
			return &g_config->m_zero_var;
		}

		// variant_set()

		void type_type::variant_set(any_const_pointer p_any, t_variant & p_variant) { p_variant = p_any->m_value.m_type; }
		void type_bool::variant_set(any_const_pointer p_any, t_variant & p_variant) { p_variant = p_any->m_value.m_bool; }
		void type_int::variant_set(any_const_pointer p_any, t_variant & p_variant) { p_variant = p_any->m_value.m_int; }
		void type_float::variant_set(any_const_pointer p_any, t_variant & p_variant) { p_variant = p_any->m_value.m_float; }
		void type_text::variant_set(any_const_pointer p_any, t_variant & p_variant) {
			p_variant = p_any->m_value.m_compound->get_text();
		}
		void type_array::variant_set(any_const_pointer p_any, t_variant & p_variant) {
			p_variant = p_any->m_value.m_compound->get_array();
		}
		void type_map::variant_set(any_const_pointer p_any, t_variant & p_variant) {
			p_variant = p_any->m_value.m_compound->get_map();
		}
		void type_struct::variant_set(any_const_pointer p_any, t_variant & p_variant) {
			p_variant = p_any->m_value.m_compound->get_struct();
		}

	} // ns

} // ns