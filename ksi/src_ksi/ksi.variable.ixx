module;

#include "../src/pre.h"

export module ksi.var;

import <cstddef>;
import <concepts>;
export import <vector>;
export import <map>;
export import just.common;
export import just.list;
export import just.text;
export import just.array;
export import ksi.log;

export namespace ksi {

	using t_index = just::t_diff;

	namespace var {

		using namespace just::text_literals;

		using t_integer = just::t_int_max;
		using t_floating = double;
		using t_text_value = just::text;
		using t_text_value_pointer = t_text_value *;

		using log_pointer = log_base *;
		using output_pointer = just::output_base *;

		struct compound_base;
		struct compound_text;
		struct compound_array;
		struct compound_struct;
		using compound_pointer = compound_base *;
		using compound_text_pointer = compound_text *;
		using compound_array_pointer = compound_array *;
		using compound_struct_pointer = compound_struct *;

		struct static_data;
		using static_data_pointer = static_data *;

		struct static_data_base {
			virtual ~static_data_base() = default;

			virtual void init() {}
		};

		struct with_owner {
			// data
			compound_pointer	m_owner = nullptr;
		};

		struct owner_node :
			public with_owner,
			public just::node_list<owner_node>,
			public just::bases::with_deleter<owner_node *>
		{
			using t_node = just::node_list<owner_node>;
		};

		using owner_node_pointer = owner_node *;

		struct any;
		using any_pointer = any *;
		using any_const_pointer = const any *;

		struct any_var;
		using var_pointer = any_var *;
		using var_const_pointer = const any_var *;

		struct any_link;
		using link_pointer = any_link *;

		// types

		struct type_base {
			using t_static = std::unique_ptr<static_data_base>;

			// data
			bool			m_is_compound	= false;
			bool			m_is_struct		= false;
			t_text_value	m_name;
			t_static		m_static;

			void init_base();
			void init() {
				init_base();
				m_static->init();
			}
			static_data_pointer get_static();

			virtual compound_pointer	var_owner(var_const_pointer p_var) = 0;
			virtual void				var_owner_set(var_pointer p_var, compound_pointer p_owner) = 0;
			virtual link_pointer		link_make_maybe(var_pointer p_var) = 0;
			virtual any_pointer			any_get(any_pointer p_any) = 0;
			virtual any_const_pointer	any_get_const(any_const_pointer p_any) = 0;
			virtual void				any_close(any_pointer p_any) = 0;
			virtual void				var_change(var_pointer p_to, any_const_pointer p_from) = 0;
			//
			virtual bool write(any_const_pointer p_any, output_pointer p_out) { return true; }
			virtual var_pointer element(any_pointer p_any, const any & p_key, bool & p_wrong_key);
			virtual var_pointer element_const(any_pointer p_any, const t_text_value & p_key, bool & p_wrong_key);
		};

		using type_pointer = type_base *;

		struct type_link :
			public type_base
		{
			type_link() {
				using namespace just::text_literals;
				m_name = "$link#"_jt;
			}

			auto var_owner(var_const_pointer p_var) -> compound_pointer override;
			void var_owner_set(var_pointer p_var, compound_pointer p_owner) override;
			auto link_make_maybe(var_pointer p_var) -> link_pointer override;
			auto any_get(any_pointer p_any) -> any_pointer override;
			auto any_get_const(any_const_pointer p_any) -> any_const_pointer override;
			void any_close(any_pointer p_any) override;
			void var_change(var_pointer p_to, any_const_pointer p_from) override;
			bool write(any_const_pointer p_any, output_pointer p_out) override;
		};

		struct type_ref :
			public type_link
		{
			type_ref() {
				using namespace just::text_literals;
				m_name = "$ref#"_jt;
			}

			void var_change(var_pointer p_to, any_const_pointer p_from) override;
		};

		// simple

		struct type_simple :
			public type_base
		{
			auto var_owner(var_const_pointer p_var) -> compound_pointer override;
			void var_owner_set(var_pointer p_var, compound_pointer p_owner) override;
			auto link_make_maybe(var_pointer p_var) -> link_pointer override;
			auto any_get(any_pointer p_any) -> any_pointer override;
			auto any_get_const(any_const_pointer p_any) -> any_const_pointer override;
			void any_close(any_pointer p_any) override;
			void var_change(var_pointer p_to, any_const_pointer p_from) override;
		};

		struct type_null :
			public type_simple
		{
			type_null() {
				using namespace just::text_literals;
				m_name = "$null#"_jt;
			}
		};

		struct type_type :
			public type_simple
		{
			type_type() {
				using namespace just::text_literals;
				m_name = "$type#"_jt;
			}

			auto element(any_pointer p_any, const any & p_key, bool & p_wrong_key) -> var_pointer override;
			auto element_const(any_pointer p_any, const t_text_value & p_key, bool & p_wrong_key) -> var_pointer override;
		};

		struct type_bool :
			public type_simple
		{
			type_bool() {
				using namespace just::text_literals;
				m_name = "$bool#"_jt;
			}

			bool write(any_const_pointer p_any, output_pointer p_out) override;
		};

		struct type_int :
			public type_simple
		{
			type_int() {
				using namespace just::text_literals;
				m_name = "$int#"_jt;
			}

			bool write(any_const_pointer p_any, output_pointer p_out) override;
		};

		struct type_float :
			public type_simple
		{
			type_float() {
				using namespace just::text_literals;
				m_name = "$float#"_jt;
			}

			bool write(any_const_pointer p_any, output_pointer p_out) override;
		};

		// compound

		struct type_compound :
			public type_base
		{
			type_compound() {
				m_is_compound = true;
			}

			auto var_owner(var_const_pointer p_var) -> compound_pointer override;
			void var_owner_set(var_pointer p_var, compound_pointer p_owner) override;
			auto link_make_maybe(var_pointer p_var) -> link_pointer override;
			auto any_get(any_pointer p_any) -> any_pointer override;
			auto any_get_const(any_const_pointer p_any) -> any_const_pointer override;
			void any_close(any_pointer p_any) override;
			void var_change(var_pointer p_to, any_const_pointer p_from) override;
			static void link_change(link_pointer p_to, any_const_pointer p_from);
		};

		struct type_text :
			public type_compound
		{
			type_text() {
				using namespace just::text_literals;
				m_name = "$text#"_jt;
			}

			auto element_const(any_pointer p_any, const t_text_value & p_key, bool & p_wrong_key) -> var_pointer override;
		};

		struct type_array :
			public type_compound
		{
			type_array() {
				using namespace just::text_literals;
				m_name = "$array#"_jt;
			}

			auto element(any_pointer p_any, const any & p_key, bool & p_wrong_key) -> var_pointer override;
			auto element_const(any_pointer p_any, const t_text_value & p_key, bool & p_wrong_key) -> var_pointer override;
		};

		struct type_struct :
			public type_compound,
			public just::node_list<type_struct>,
			public just::bases::with_deleter<type_struct *>
		{
			using t_map = std::map<t_text_value, t_integer, just::text_less>;
			using t_insert = std::pair<t_map::iterator, bool>;

			// data
			t_map	m_props;

			type_struct(const t_text_value & p_name) {
				m_is_struct = true;
				m_name = p_name;
			}

			bool prop_add(const t_text_value & p_prop_name) {
				t_integer v_index = props_count();
				t_insert v_res = m_props.insert({p_prop_name, v_index});
				return v_res.second;
			}

			t_integer props_count() const { return std::ssize(m_props); }

			auto element(any_pointer p_any, const any & p_key, bool & p_wrong_key) -> var_pointer override;
			auto element_const(any_pointer p_any, const t_text_value & p_key, bool & p_wrong_key) -> var_pointer override;
		};

		using type_struct_pointer = type_struct *;

		// config

		struct config {
			// data
			fs::path		m_path;
			log_list		m_log_system;
			log_pointer		m_log = &m_log_system;
			output_pointer	m_out = &just::g_console;
			bool			m_wrong_key_notice = false;
			//
			type_null		m_null;
			type_link		m_link;
			type_ref		m_ref;
			type_type		m_type;
			type_bool		m_bool;
			type_int		m_int;
			type_float		m_float;
			type_text		m_text;
			type_array		m_array;
			//
			var_pointer		m_zero_var = nullptr;

			static config * instance();
			void init() {
				static bool v_once = false;
				if( v_once ) { return; }
				else { v_once = true; }
				m_null	.init();
				m_link	.init();
				m_ref	.init();
				m_type	.init();
				m_bool	.init();
				m_int	.init();
				m_float	.init();
				m_text	.init();
				m_array	.init();
			}
		};

		using config_pointer = config *;
		config_pointer g_config = nullptr;

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
			type_pointer		m_type;
			link_pointer		m_link;
			compound_pointer	m_compound;
		};

		struct any {
			// data
			any_value		m_value;
			type_pointer	m_type;

			any(const any &) = delete;
			any(any &&) = delete;

			any & operator = (const any &) = delete;
			any & operator = (any &&) = delete;

			~any() { close(); }

			any() : m_type{&g_config->m_null} {}
			any(std::nullptr_t, type_null * p_type_null) : m_type{p_type_null} {}
			any(bool p_value) : m_type{&g_config->m_bool} {
				m_value.m_bool = p_value;
			}
			any(t_integer p_value) : m_type{&g_config->m_int} {
				m_value.m_int = p_value;
			}
			any(t_floating p_value) : m_type{&g_config->m_float} {
				m_value.m_float = p_value;
			}
			any(type_pointer p_value) : m_type{&g_config->m_type} {
				m_value.m_type = p_value;
			}

			auto any_get() -> any_pointer { return m_type->any_get(this); }
			auto any_get_const() const -> any_const_pointer { return m_type->any_get_const(this); }
			auto type_get() const -> type_pointer { return any_get_const()->m_type; }
			void close() { m_type->any_close(this); }
			bool write(output_pointer p_out = g_config->m_out) const {
				return m_type->write(this, p_out);
			}

			var_pointer element(const any & p_key, bool & p_wrong_key) {
				return m_type->element(this, p_key, p_wrong_key);
			}
			var_pointer element_const(const t_text_value & p_key, bool & p_wrong_key) {
				return m_type->element_const(this, p_key, p_wrong_key);
			}
		};

		inline just::output_base & operator , (just::output_base & p_out, any_const_pointer p_value) {
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
				owner_node_pointer	m_owner_node;
			};

			any_var() = default;
			any_var(const t_text_value & p_text);
			any_var(type_struct_pointer p_type);

			any_var(const any_var & p_other) : any() { // copy
				any_const_pointer v_from = p_other.any_get_const();
				v_from->m_type->var_change(this, v_from);
			}
			any_var(any_var && p_other) : any() { // move
				p_other.m_type->var_change(this, &p_other);
			}

			any_var & operator = (const any_var & p_other) { // copy assign
				any_const_pointer v_from = p_other.any_get_const();
				v_from->m_type->var_change(this, v_from);
				return *this;
			}
			any_var & operator = (any_var && p_other) { // move assign
				p_other.m_type->var_change(this, &p_other);
				return *this;
			}

			auto var_owner() -> compound_pointer { return m_type->var_owner(this); }
			void var_owner_set(compound_pointer p_owner) { m_type->var_owner_set(this, p_owner); }
			auto link_make_maybe() -> link_pointer { return m_type->link_make_maybe(this); }

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
			public just::bases::with_deleter<any_link *>
		{
			// data
			owner_node::t_node	m_owners;

			any_link() = default;

			any_link(const any_link &) = delete; // no copy
			any_link(any_link &&) = delete; // no move
			any_link & operator = (const any_link &) = delete; // no copy assign
			any_link & operator = (any_link &&) = delete; // no move assign

			compound_pointer link_owner() {
				return m_owners.m_next->node_get_target()->m_owner;
			}
		};

		using link_node = just::node_list<any_link>;
		using link_node_pointer = link_node *;

		config * config::instance() {
			static config v_inst;
			static any_var v_zero_var(nullptr, &v_inst.m_null);
			if( v_inst.m_zero_var == nullptr ) {
				v_inst.m_zero_var = &v_zero_var;
			}
			return &v_inst;
		}

		// compound_base

		struct compound_base :
			public just::bases::with_deleter<compound_base *>
		{
			using t_node = just::node_list<any_link>;
			// data
			t_node	m_links_strong, m_links_weak;

			virtual ~compound_base() = default;

			compound_text_pointer	get_text();
			compound_array_pointer	get_array();
			compound_struct_pointer	get_struct();

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

		// compound_text

		struct compound_text :
			public compound_base
		{
			// data
			t_text_value	m_text;
			any_var			m_count;

			compound_text(const t_text_value & p_text) : m_text(p_text) {}

			void link_text(link_pointer p_link) {
				// strong link
				m_links_strong.m_prev->node_attach(p_link);
			}

			t_integer count() { return m_text->size(); }
		};

		inline compound_text_pointer compound_base::get_text() { return static_cast<compound_text_pointer>(this); }

		any_var::any_var(const t_text_value & p_text) : any{} {
			link_pointer v_link = link_make_maybe();
			v_link->m_type = &g_config->m_text;
			compound_text_pointer v_compound_text;
			v_link->m_value.m_compound = v_compound_text = new compound_text(p_text);
			v_compound_text->link_text(v_link);
		}

		// compound_with_lock

		struct compound_with_lock :
			public compound_base
		{
			// data
			t_integer	m_lock;

			void lock_add() { ++m_lock; }
			void lock_del() { --m_lock; }
			bool lock_check() { return m_lock; }
		};

		// compound_array

		struct compound_array :
			public compound_with_lock
		{
			//using t_items = std::vector<any_var>;
			using t_items = just::array_alias<var::any_var, just::capacity_step<8, 8> >;

			// data
			t_items		m_items;
			any_var		m_count;

			compound_array(t_integer p_count) {
				if( p_count ) {
					just::array_append_n(m_items, p_count);
					for( any_var & v_it : m_items ) {
						v_it.var_owner_set(this);
					}
				}
			}

			t_integer count() { return m_items->m_count; }
		};

		inline compound_array_pointer compound_base::get_array() { return static_cast<compound_array_pointer>(this); }

		// compound_struct

		struct compound_struct :
			public compound_with_lock
		{
			using t_items = std::vector<any_var>;

			// data
			type_struct_pointer		m_type;
			t_items					m_items;
			any_var					m_count;

			compound_struct(type_struct_pointer p_type) : m_type{p_type} {
				t_integer v_count = 0;
				if( p_type->m_is_struct ) {
					type_struct_pointer v_type_struct = static_cast<type_struct_pointer>(p_type);
					v_count = v_type_struct->props_count();
				}
				if( v_count ) {
					{
						t_items v_items(v_count);
						std::ranges::swap(m_items, v_items);
					}
					for( any_var & v_it : m_items ) {
						v_it.var_owner_set(this);
					}
				}
			}
		};

		inline compound_struct_pointer compound_base::get_struct() { return static_cast<compound_struct_pointer>(this); }

		any_var::any_var(type_struct_pointer p_type) : any{} {
			link_pointer v_link = link_make_maybe();
			v_link->m_type = p_type;
			compound_struct_pointer v_compound;
			v_link->m_value.m_compound = v_compound = new compound_struct(p_type);
			v_compound->link(v_link);
		}

		// static_data

		struct static_data :
			public static_data_base
		{
			// data
			type_struct		m_struct_props, m_struct_consts;
			any_var			m_props, m_consts;

			static_data() : m_struct_props("$static#"_jt), m_struct_consts("$static#"_jt) {}

			void init() override {
				m_props = any_var(&m_struct_props);
				m_consts = any_var(&m_struct_consts);
			}
		};

		void type_base::init_base() {
			m_static = std::make_unique<static_data>();
		}

		static_data_pointer type_base::get_static() {
			return static_cast<static_data_pointer>(m_static.get() );
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
			/*using namespace just::text_literals;
			g_config->m_log->add({g_config->m_path,
				"system bad case: any_get() was called for compound type."_jt
			});*/
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
			/*using namespace just::text_literals;
			g_config->m_log->add({g_config->m_path,
				"system bad case: any_get_const() was called for compound type."_jt
			});*/
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
			g_config->m_zero_var->close();
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

		bool type_link::write(any_const_pointer p_any, output_pointer p_out) {
			return p_any->m_value.m_link->write(p_out);
		}

		bool type_bool::write(any_const_pointer p_any, output_pointer p_out) {
			return p_out->write(p_any->m_value.m_bool ? 1 : 0);
		}

		bool type_int::write(any_const_pointer p_any, output_pointer p_out) {
			return p_out->write(p_any->m_value.m_int);
		}

		bool type_float::write(any_const_pointer p_any, output_pointer p_out) {
			return p_out->write(p_any->m_value.m_float);
		}

		// element()

		var_pointer type_base::element(any_pointer p_any, const any & p_key, bool & p_wrong_key) {
			p_wrong_key = true;
			return g_config->m_zero_var;
		}

		var_pointer type_type::element(any_pointer p_any, const any & p_key, bool & p_wrong_key) {
			static_data_pointer v_static = p_any->m_type->get_static();
			any_pointer v_any = v_static->m_props.any_get();
			return v_any->element(p_key, p_wrong_key);
		}

		var_pointer type_array::element(any_pointer p_any, const any & p_key, bool & p_wrong_key) {
			compound_array_pointer v_array = p_any->m_value.m_compound->get_array();
			if( t_integer v_count = v_array->count() ) {
				any_const_pointer v_any_key = p_key.any_get_const();
				if( v_any_key->m_type == &g_config->m_int ) { // int key
					t_integer v_key = v_any_key->m_value.m_int;
					if( v_key >= 0 && v_key < v_count ) {
						p_wrong_key = false;
						return v_array->m_items.data() + v_key;
					}
				} else if( v_any_key->m_type == &g_config->m_text ) { // text key
					t_text_value_pointer v_key = &v_any_key->m_value.m_compound->get_text()->m_text;
					if( just::text_traits::cmp((*v_key)->m_text, "last") == 0 ) { // last
						p_wrong_key = false;
						return v_array->m_items.data() + v_count -1;
					}
				}
			}
			p_wrong_key = true;
			return g_config->m_zero_var;
		}

		var_pointer type_struct::element(any_pointer p_any, const any & p_key, bool & p_wrong_key) {
			if( m_props.size() ) {
				compound_struct_pointer v_struct = p_any->m_value.m_compound->get_struct();
				any_const_pointer v_any_key = p_key.any_get_const();
				if( v_any_key->m_type == &g_config->m_int ) { // int key
					t_integer v_key = v_any_key->m_value.m_int;
					if( v_key >= 0 && v_key < props_count() ) {
						p_wrong_key = false;
						return v_struct->m_items.data() + v_key;
					}
				} else if( v_any_key->m_type == &g_config->m_text ) { // text key
					t_text_value_pointer v_key = &v_any_key->m_value.m_compound->get_text()->m_text;
					typename t_map::iterator v_it = m_props.find(*v_key);
					if( v_it != m_props.end() ) {
						p_wrong_key = false;
						return v_struct->m_items.data() + (*v_it).second;
					}
				}
			}
			p_wrong_key = true;
			return g_config->m_zero_var;
		}

		// element_const()

		var_pointer type_base::element_const(any_pointer p_any, const t_text_value & p_key, bool & p_wrong_key) {
			p_wrong_key = true;
			return g_config->m_zero_var;
		}

		var_pointer type_type::element_const(any_pointer p_any, const t_text_value & p_key, bool & p_wrong_key) {
			static_data_pointer v_static = p_any->m_type->get_static();
			any_pointer v_any = v_static->m_consts.any_get();
			any_var v_key{p_key};
			return v_any->element(v_key, p_wrong_key);
		}

		var_pointer type_text::element_const(any_pointer p_any, const t_text_value & p_key, bool & p_wrong_key) {
			compound_text_pointer v_compound = p_any->m_value.m_compound->get_text();
			if( just::text_traits::cmp(p_key->m_text, "count#") == 0 ) { // count
				v_compound->m_count = v_compound->count();
				p_wrong_key = false;
				return &v_compound->m_count;
			}
			p_wrong_key = true;
			return g_config->m_zero_var;
		}

		var_pointer type_array::element_const(any_pointer p_any, const t_text_value & p_key, bool & p_wrong_key) {
			compound_array_pointer v_compound = p_any->m_value.m_compound->get_array();
			if( just::text_traits::cmp(p_key->m_text, "count#") == 0 ) { // count
				v_compound->m_count = v_compound->count();
				p_wrong_key = false;
				return &v_compound->m_count;
			}
			p_wrong_key = true;
			return g_config->m_zero_var;
		}

		var_pointer type_struct::element_const(any_pointer p_any, const t_text_value & p_key, bool & p_wrong_key) {
			compound_struct_pointer v_compound = p_any->m_value.m_compound->get_struct();
			if( just::text_traits::cmp(p_key->m_text, "count#") == 0 ) { // count
				v_compound->m_count = props_count();
				p_wrong_key = false;
				return &v_compound->m_count;
			}
			p_wrong_key = true;
			return g_config->m_zero_var;
		}

	} // ns

} // ns