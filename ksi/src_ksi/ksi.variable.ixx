module;

#include "../src/pre.h"

export module ksi.var;

import <cstddef>;
import <concepts>;
import <limits>;
export import <vector>;
export import <map>;
export import just.common;
export import just.list;
export import just.text;
export import just.array;
export import ksi.log;

export namespace ksi {

	using namespace just::text_literals;

	using t_index = just::t_diff;

	namespace var {

		using t_integer = just::t_int_max;
		using t_floating = double;

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
			virtual var_pointer element(any_pointer p_any, any_const_pointer p_key, bool & p_wrong_key);
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

			auto element(any_pointer p_any, any_const_pointer p_key, bool & p_wrong_key) -> var_pointer override;
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
			using t_limits = std::numeric_limits<t_integer>;

			type_int() {
				using namespace just::text_literals;
				m_name = "$int#"_jt;
			}

			void init();
			bool write(any_const_pointer p_any, output_pointer p_out) override;
		};

		struct type_float :
			public type_simple
		{
			using t_limits = std::numeric_limits<t_floating>;

			type_float() {
				using namespace just::text_literals;
				m_name = "$float#"_jt;
			}

			void init();
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

			auto element(any_pointer p_any, any_const_pointer p_key, bool & p_wrong_key) -> var_pointer override;
			auto element_const(any_pointer p_any, const t_text_value & p_key, bool & p_wrong_key) -> var_pointer override;
		};

		struct type_struct;
		using type_struct_pointer = type_struct *;

		// config

		struct config;

		using config_pointer = config *;
		config_pointer g_config = nullptr;

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

			virtual ~any() { close(); }

			any(std::nullptr_t, type_null * p_type_null) : m_type{p_type_null} {}
			any(); // $null#
			any(bool p_value); // $bool#
			any(t_integer p_value); // $int#
			any(t_floating p_value); // $float#
			any(type_pointer p_value); // $type#

			auto any_get() -> any_pointer { return m_type->any_get(this); }
			auto any_get_const() const -> any_const_pointer { return m_type->any_get_const(this); }
			auto type_get() const -> type_pointer { return any_get_const()->m_type; }
			void close() { m_type->any_close(this); }
			bool write(output_pointer p_out) const {
				return m_type->write(this, p_out);
			}

			var_pointer element(const t_text_value & p_key, bool & p_wrong_key);
			var_pointer element(const any & p_key, bool & p_wrong_key) {
				return m_type->element(any_get(), p_key.any_get_const(), p_wrong_key);
			}
			var_pointer element_const(const t_text_value & p_key, bool & p_wrong_key) {
				return m_type->element_const(any_get(), p_key, p_wrong_key);
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
			any_var(const t_text_value & p_text); // $text#
			any_var(type_struct_pointer p_type);

			any_var(const any_var & p_other); // copy
			any_var(any_var && p_other); // move

			any_var & operator = (const any_var & p_other); // copy assign
			any_var & operator = (any_var && p_other); // move assign

			auto var_owner() -> compound_pointer { return m_type->var_owner(this); }
			void var_owner_set(compound_pointer p_owner) { m_type->var_owner_set(this, p_owner); }
			auto link_make_maybe() -> link_pointer { return m_type->link_make_maybe(this); }

			void link_to(var_pointer p_var);
			void ref_to(var_pointer p_var);
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

		//

		struct type_struct :
			public type_compound,
			public just::node_list<type_struct>,
			public just::bases::with_deleter<type_struct *>
		{
			using t_map = std::map<t_text_value, t_integer, just::text_less>;
			using t_insert = std::pair<t_map::iterator, bool>;
			using t_default = just::array_alias<var::any_var, just::capacity_step<8, 8> >;

			// data
			t_map		m_props;
			t_default	m_default;

			type_struct(const t_text_value & p_name) {
				m_is_struct = true;
				m_name = p_name;
			}

			bool prop_add(const t_text_value & p_prop_name, const any_var & p_default = any_var{}) {
				t_integer v_index = props_count();
				t_insert v_res = m_props.insert({p_prop_name, v_index});
				if( v_res.second ) {
					just::array_append(m_default, p_default);
				}
				return v_res.second;
			}

			t_integer props_count() const { return std::ssize(m_props); }

			auto element(any_pointer p_any, any_const_pointer p_key, bool & p_wrong_key) -> var_pointer override;
			auto element_const(any_pointer p_any, const t_text_value & p_key, bool & p_wrong_key) -> var_pointer override;
		};

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
					for( t_integer v_index = 0; v_index < v_count; ++v_index ) {
						m_items[v_index].var_owner_set(this);
						m_items[v_index] = p_type->m_default[v_index];
					}
				}
			}

			t_integer count() { return m_items.size(); }
		};

		inline compound_struct_pointer compound_base::get_struct() { return static_cast<compound_struct_pointer>(this); }

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

		// init()

		void type_base::init_base() {
			m_static = std::make_unique<static_data>();
		}

		static_data_pointer type_base::get_static() {
			return static_cast<static_data_pointer>(m_static.get() );
		}

		void type_int::init() {
			using namespace just::text_literals;
			init_base();
			static_data_pointer v_static = get_static();
			v_static->m_struct_consts.prop_add("min#"_jt, t_limits::min() );
			v_static->m_struct_consts.prop_add("max#"_jt, t_limits::max() );
			v_static->init();
		}

		void type_float::init() {
			using namespace just::text_literals;
			init_base();
			static_data_pointer v_static = get_static();
			v_static->m_struct_consts.prop_add("min#"_jt,				t_limits::min() );
			v_static->m_struct_consts.prop_add("max#"_jt,				t_limits::max() );
			v_static->m_struct_consts.prop_add("infinity#"_jt,			t_limits::infinity() );
			v_static->m_struct_consts.prop_add("infinity_negative#"_jt,	-t_limits::infinity() );
			v_static->m_struct_consts.prop_add("nan#"_jt,				t_limits::quiet_NaN() );
			v_static->m_struct_consts.prop_add("epsilon#"_jt,			t_limits::epsilon() );
			v_static->init();
		}

		// element()

		inline var_pointer any::element(const t_text_value & p_key, bool & p_wrong_key) {
			any_var v_key{p_key};
			return element(v_key, p_wrong_key);
		}

	} // ns

} // ns