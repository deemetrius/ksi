module;

#include "../src/pre.h"

export module ksi.var;

import <concepts>;
export import just.common;
export import just.aux;
export import just.list;
export import just.output;
export import ksi.log;

export namespace ksi {

	namespace var {

		using t_integer = just::t_int_max;
		using t_floating = double;

		using log_pointer = log_base *;
		using output_pointer = just::output_base *;

		struct compound_base;
		using compound_pointer = compound_base *;

		struct with_owner {
			// data
			compound_pointer	m_owner = nullptr;
		};

		struct owner_node :
			public with_owner,
			public just::node_list<owner_node>,
			public just::bases::with_deleter<owner_node>
		{
			using t_node = just::node_list<owner_node>;
		};

		using owner_node_pointer = owner_node *;

		struct any;
		using any_pointer = any *;

		struct any_var;
		using var_pointer = any_var *;

		struct any_link;
		using link_pointer = any_link *;

		// types

		struct type_base {
			// data
			bool	m_is_compound = false;

			virtual compound_pointer	var_owner(var_pointer p_var) = 0;
			virtual void				var_owner_set(var_pointer p_var, compound_pointer p_owner) = 0;
			virtual link_pointer		link_make_maybe(var_pointer p_var) = 0;
			virtual any_pointer			any_get(var_pointer p_var) = 0;
			virtual void				any_close(any_pointer p_any) = 0;
			virtual void				var_change(var_pointer p_to, any_pointer p_from) = 0;
			//
			virtual bool				write(any_pointer p_any, output_pointer p_out) { return true; }
		};

		using type_pointer = type_base *;

		struct type_link :
			public type_base
		{
			compound_pointer	var_owner(var_pointer p_var) override;
			void				var_owner_set(var_pointer p_var, compound_pointer p_owner) override;
			link_pointer		link_make_maybe(var_pointer p_var) override;
			any_pointer			any_get(var_pointer p_var) override;
			void				any_close(any_pointer p_any) override;
			void				var_change(var_pointer p_to, any_pointer p_from) override;
			bool				write(any_pointer p_any, output_pointer p_out) override;
		};

		struct type_ref :
			public type_link
		{
			void var_change(var_pointer p_to, any_pointer p_from) override;
		};

		// simple

		struct type_simple :
			public type_base
		{
			compound_pointer	var_owner(var_pointer p_var) override;
			void				var_owner_set(var_pointer p_var, compound_pointer p_owner) override;
			link_pointer		link_make_maybe(var_pointer p_var) override;
			any_pointer			any_get(var_pointer p_var) override;
			void				any_close(any_pointer p_any) override;
			void				var_change(var_pointer p_to, any_pointer p_from) override;
		};

		struct type_null :
			public type_simple
		{};

		struct type_bool :
			public type_simple
		{
			bool write(any_pointer p_any, output_pointer p_out) override;
		};

		struct type_int :
			public type_simple
		{
			bool write(any_pointer p_any, output_pointer p_out) override;
		};

		struct type_float :
			public type_simple
		{
			bool write(any_pointer p_any, output_pointer p_out) override;
		};

		// compound

		struct type_compound :
			public type_base
		{
			type_compound() {
				m_is_compound = true;
			}

			compound_pointer	var_owner(var_pointer p_var) override;
			void				var_owner_set(var_pointer p_var, compound_pointer p_owner) override;
			link_pointer		link_make_maybe(var_pointer p_var) override;
			any_pointer			any_get(var_pointer p_var) override;
			void				any_close(any_pointer p_any) override;
			void				var_change(var_pointer p_to, any_pointer p_from) override;
		};

		struct type_array :
			public type_compound
		{};

		// config

		struct config {
			// data
			fs::path		m_path;
			log_list		m_log_system;
			log_pointer		m_log = &m_log_system;
			output_pointer	m_out = &just::g_console;
			//
			type_null		m_null;
			type_link		m_link;
			type_ref		m_ref;
			type_bool		m_bool;
			type_int		m_int;
			type_float		m_float;
			type_array		m_array;

			static config * instance() {
				static config inst;
				return &inst;
			}
		};

		using config_pointer = config *;
		config_pointer g_config = config::instance();

		struct log_switcher {
			// data
			log_pointer		m_prev_log;

			log_switcher(log_pointer p_log) {
				m_prev_log = g_config->m_log;
				g_config->m_log = p_log;
			}

			~log_switcher() {
				g_config->m_log = m_prev_log;
			}
		};

		// any

		union any_value {
			// data
			t_integer			m_int;
			t_floating			m_float;
			bool				m_bool;
			link_pointer		m_link;
			compound_pointer	m_compound;
		};

		struct any {
			// data
			any_value		m_value;
			type_pointer	m_type;

			any(const any &) = delete;
			any(any &&) = delete;
			~any() { close(); }

			any() : m_type{&g_config->m_null} {}
			any(bool p_value) : m_type{&g_config->m_bool} {
				m_value.m_bool = p_value;
			}
			any(t_integer p_value) : m_type{&g_config->m_int} {
				m_value.m_int = p_value;
			}
			any(t_floating p_value) : m_type{&g_config->m_float} {
				m_value.m_float = p_value;
			}

			void close() { m_type->any_close(this); }
			bool write(output_pointer p_out = g_config->m_out) { return m_type->write(this, p_out); }
		};

		inline just::output_base & operator , (just::output_base & p_out, any_pointer p_value) {
			p_value->write(&p_out);
			return p_out;
		}

		struct any_var :
			public any
		{
			using any::any;

			// data
			union {
				compound_pointer	m_owner = nullptr;
				owner_node_pointer		m_owner_node;
			};

			any_var() = default;

			any_var(const any_var &) = delete; // no copy
			any_var(any_var && p_other) : any() { // move
				p_other.m_type->var_change(this, &p_other);
			}
			any_var(any_var & p_other) : any() {
				any_pointer v_from = p_other.any_get();
				v_from->m_type->var_change(this, v_from);
			}

			any_var & operator = (const any_var &) = delete; // no copy assign
			any_var & operator = (any_var && p_other) { // move assign
				p_other.m_type->var_change(this, &p_other);
				return *this;
			}
			any_var & operator = (any_var & p_other) {
				any_pointer v_from = p_other.any_get();
				v_from->m_type->var_change(this, v_from);
				return *this;
			}

			compound_pointer var_owner() { return m_type->var_owner(this); }
			void var_owner_set(compound_pointer p_owner) { m_type->var_owner_set(this, p_owner); }
			any_pointer any_get() { return m_type->any_get(this); }
			link_pointer link_make_maybe() { return m_type->link_make_maybe(this); }

			template <bool C_is_ref = false>
			void link_to(var_pointer p_var) {
				p_var->link_make_maybe();
				g_config->m_ref.var_change(this, p_var);
				if constexpr( C_is_ref ) {
					m_type = &g_config->m_ref;
				}
			}
		};

		struct any_link :
			public any,
			public just::node_list<any_link>,
			public just::bases::with_deleter<any_link>
		{
			// data
			owner_node::t_node	m_owners;

			compound_pointer link_owner() {
				return m_owners.m_next->node_get_target()->m_owner;
			}
		};

		using link_node = just::node_list<any_link>;
		using link_node_pointer = link_node *;

		// compound_base

		struct compound_base :
			public just::bases::with_deleter<compound_base>
		{
			using t_node = just::node_list<any_link>;
			// data
			t_node	m_links_strong, m_links_weak;

			virtual ~compound_base() = default;

			bool link_is_primary(link_pointer v_link) { return m_links_strong.m_next == v_link; }

			link_pointer link_get_primary() {
				return m_links_strong.node_empty() ? nullptr :
					m_links_strong.m_next->node_get_target()
				;
			}

			void link(link_pointer p_link) {
				compound_pointer v_source;
				while( v_source = p_link->link_owner() ) {
					if( v_source == this ) {
						// weak link
						m_links_weak.m_prev->node_attach(p_link);
						return;
					}
					p_link = v_source->link_get_primary();
					if( !p_link ) break;
				}
				// strong link
				m_links_strong.m_prev->node_attach(p_link);
			}
		};

		// var_owner()

		compound_pointer type_link::var_owner(var_pointer p_var) {
			return p_var->m_owner_node->m_owner;
		}

		compound_pointer type_simple::var_owner(var_pointer p_var) {
			return p_var->m_owner;
		}

		compound_pointer type_compound::var_owner(var_pointer p_var) {
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

		any_pointer type_link::any_get(var_pointer p_var) {
			return p_var->m_value.m_link;
		}

		any_pointer type_simple::any_get(var_pointer p_var) {
			return p_var;
		}

		any_pointer type_compound::any_get(var_pointer p_var) {
			using namespace just::text_literals;
			g_config->m_log->add({g_config->m_path,
				"system bad case: any_get() was called for compound type."_jt
			});
			return p_var;
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
					v_link->close();
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
					v_compound->m_links_weak.apply_to_others(
						[](link_node_pointer p_node, link_node_pointer p_first){
							p_node->node_get_target()->close();
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

		void type_link::var_change(var_pointer p_to, any_pointer p_from) {
			link_pointer v_link = p_from->m_value.m_link;
			v_link->m_type->var_change(p_to, v_link);
		}

		void type_ref::var_change(var_pointer p_to, any_pointer p_from) {
			p_to->close();
			link_pointer v_link = p_from->m_value.m_link;
			// update var
			p_to->m_value.m_link = v_link;
			p_to->m_type = &g_config->m_link;
			// owner_node
			p_to->m_owner_node = new owner_node{p_to->m_owner};
			v_link->m_owners.m_prev->node_attach(p_to->m_owner_node);
		}

		void type_simple::var_change(var_pointer p_to, any_pointer p_from) {
			any_pointer v_any = p_to->any_get();
			v_any->close();
			v_any->m_value = p_from->m_value;
			v_any->m_type = p_from->m_type;
		}

		void type_compound::var_change(var_pointer p_to, any_pointer p_from) {
			link_pointer v_link = p_to->link_make_maybe();
			v_link->close();
			compound_pointer v_compound = p_from->m_value.m_compound;
			v_link->m_value.m_compound = v_compound;
			v_compound->link(v_link);
			v_link->m_type = p_from->m_type;
		}

		// write()

		bool type_link::write(any_pointer p_any, output_pointer p_out) {
			return p_any->m_value.m_link->write(p_out);
		}

		bool type_bool::write(any_pointer p_any, output_pointer p_out) {
			return p_out->write(p_any->m_value.m_bool ? 1 : 0);
		}

		bool type_int::write(any_pointer p_any, output_pointer p_out) {
			return p_out->write(p_any->m_value.m_int);
		}

		bool type_float::write(any_pointer p_any, output_pointer p_out) {
			return p_out->write(p_any->m_value.m_float);
		}

	} // ns

} // ns