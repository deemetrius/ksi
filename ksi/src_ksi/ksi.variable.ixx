module;

#include "../src/pre.h"

export module ksi.var;

import <cstddef>;
import <concepts>;
import <limits>;
export import <vector>;
export import <set>;
export import <map>;
export import <variant>;
export import just.common;
export import just.list;
export import just.text;
export import just.hive;
export import just.map;
export import ksi.log;

export namespace ksi {

	using namespace just::text_literals;
	using namespace std::literals::string_view_literals;

	using t_size		= just::t_size;
	using t_index		= just::t_index;
	using t_int_ptr		= just::t_int_ptr;
	using t_uint_ptr	= just::t_uint_ptr;
	using t_integer		= just::t_int_max;
	using t_floating	= double;

	constexpr t_int_ptr n_tab_size = 4;

	struct is_module {
		// data
		bool	m_is_global = false;

		virtual t_text_value name() const = 0;
	};
	
	using module_pointer = is_module *;

	namespace var {

		enum t_id_config : t_integer {
			n_id_standard				= 0,
			n_id_special				= 1000,
			n_id_enum					= 100'000,
			n_id_struct					= 200'000,
			n_id_delta_static_props		= 1'000'000,
			n_id_delta_static_consts	= 2'000'000,
			n_id_all					= std::numeric_limits<t_integer>::max() -100,
			n_id_cat_standard			= 0,
			n_id_cat_custom				= 1000
		};

		using output_pointer = just::output_base *;

		struct compound_base;
		struct compound_text;
		struct compound_array;
		struct compound_map;
		struct compound_struct;
		using compound_pointer			= compound_base *;
		using compound_text_pointer		= compound_text *;
		using compound_array_pointer	= compound_array *;
		using compound_map_pointer		= compound_map *;
		using compound_struct_pointer	= compound_struct *;

		using set_deep = std::set<compound_pointer>; // recursion detection
		struct set_deep_changer {
			using set_deep_pointer = set_deep *;
			
			// data
			set_deep_pointer	m_deep;
			compound_pointer	m_item;

			set_deep_changer(set_deep & p_deep, compound_pointer p_item) : m_deep{&p_deep}, m_item{p_item} {
				p_deep.insert(p_item);
			}
			~set_deep_changer() { m_deep->erase(m_item); }
		};

		struct static_data;
		using static_data_pointer = static_data *;

		struct static_data_base {
			virtual ~static_data_base() = default;

			virtual void init() {}
		};

		//

		template <typename T, typename T_compare = std::ranges::less>
		struct includes {
			using type = T;
			using t_items = just::map<T, T, T_compare>;
			using t_add_result = t_items::t_add_result;
			using t_node_pointer = t_items::t_node::pointer;

			// data
			t_items		m_direct, m_indirect;

			bool add(type p_item) {
				t_add_result v_res = m_direct.maybe_emplace(p_item, p_item);
				if( !v_res.second ) { return false; }
				m_indirect.remove(p_item);
				for( t_node_pointer v_it : p_item->m_includes.m_direct ) {
					add_indirect(v_it->m_value);
				}
				for( t_node_pointer v_it : p_item->m_includes.m_indirect ) {
					add_indirect(v_it->m_value);
				}
				return true;
			}

			bool add_indirect(type p_item) {
				if( m_direct.find(p_item) ) { return false; }
				t_add_result v_res = m_indirect.maybe_emplace(p_item, p_item);
				return v_res.second;
			}

			void add_from(includes & p_from) {
				for( t_node_pointer v_it : p_from.m_direct ) {
					if( m_indirect.find(v_it->m_value) == nullptr ) { add(v_it->m_value); }
				}
			}

			bool contains(type p_item) {
				return (m_direct.find(p_item) != nullptr) || (m_indirect.find(p_item) != nullptr);
			}
		};

		//

		struct creation_args {
			// data
			t_text_value	m_name;
			bool			m_is_local = false;
			log_pos			m_log_pos;
		};

		struct with_name {
			// data
			module_pointer	m_module;
			bool			m_is_added = false;
			t_text_value	m_name;
			t_text_value	m_name_full;

			void name(const t_text_value & p_name, bool p_hide_global = false) {
				m_name = p_name;
				if( p_hide_global && m_module->m_is_global ) { m_name_full = p_name; }
				else { m_name_full = just::implode<t_char>({p_name, m_module->name()}); }
			}
		};

		struct type_data :
			public with_name
		{
			// data
			t_integer		m_id;
			log_pos			m_log_pos;
			bool			m_is_local		= false;
			bool			m_is_global		= false;

			type_data(module_pointer p_module, t_integer & p_id) : with_name{p_module}, m_id{p_id} {
				++p_id;
			}

			void args_set(const creation_args & p_args) {
				m_is_local = p_args.m_is_local;
				m_log_pos = p_args.m_log_pos;
				name(p_args.m_name);
			}
		};

		struct category :
			public type_data,
			public just::node_list<category>,
			public just::bases::with_deleter<category *>
		{
			using pointer = category *;
			using t_includes = includes<pointer>;

			// data
			t_includes	m_includes;

			category(module_pointer p_module, t_integer & p_id, const creation_args & p_args) :
				type_data{p_module, p_id}
			{
				args_set(p_args);
			}
		};

		//

		struct variant_null {};
		struct variant_all {};

		//

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

		struct type_base :
			public type_data
		{
			using type_pointer = type_base *;
			using t_static = std::unique_ptr<static_data_base>;
			using t_variant_inner = std::variant<
				variant_null,
				variant_all,
				category::pointer,
				type_pointer,
				bool,
				t_integer,
				t_floating,
				compound_text_pointer,
				compound_array_pointer,
				compound_map_pointer,
				compound_struct_pointer
			>;

			using type_data::type_data;

			// data
			bool					m_is_compound	= false;
			bool					m_is_struct		= false;
			category::t_includes	m_categories;
			t_static				m_static;

			void init_start();
			void init_end() { m_static->init(); }
			void init() {
				init_start();
				init_target();
				init_end();
			}
			static_data_pointer get_static();

			virtual void				init_target() {}
			virtual void				init_categories() {}
			virtual compound_pointer	var_owner(var_const_pointer p_var) = 0;
			virtual void				var_owner_set(var_pointer p_var, compound_pointer p_owner) = 0;
			virtual link_pointer		link_make_maybe(var_pointer p_var) = 0;
			virtual any_pointer			any_get(any_pointer p_any) = 0;
			virtual any_const_pointer	any_get_const(any_const_pointer p_any) = 0;
			virtual void				any_close(any_pointer p_any) = 0;
			virtual void				var_change(var_pointer p_to, any_const_pointer p_from) = 0;
			//
			virtual bool write(
				output_pointer p_out,
				any_const_pointer p_any,
				const any & p_separator,
				set_deep & p_deep
			) { return true; }
			virtual var_pointer element(any_const_pointer p_any, any_const_pointer p_key, bool & p_wrong_key);
			virtual var_pointer element_const(any_const_pointer p_any, const t_text_value & p_key, bool & p_wrong_key);
			//
			virtual void variant_set(any_const_pointer p_any, t_variant_inner & p_variant) {}
		};

		using type_pointer = type_base *;
		using t_variant = type_base::t_variant_inner;

		struct type_link :
			public type_base
		{
			type_link(module_pointer p_module, t_integer & p_id) : type_base{p_module, p_id} {
				using namespace just::text_literals;
				m_is_local = true;
				name("$link#"_jt);
			}

			auto var_owner(var_const_pointer p_var) -> compound_pointer override;
			void var_owner_set(var_pointer p_var, compound_pointer p_owner) override;
			auto link_make_maybe(var_pointer p_var) -> link_pointer override;
			auto any_get(any_pointer p_any) -> any_pointer override;
			auto any_get_const(any_const_pointer p_any) -> any_const_pointer override;
			void any_close(any_pointer p_any) override;
			void var_change(var_pointer p_to, any_const_pointer p_from) override;
			bool write(
				output_pointer p_out,
				any_const_pointer p_any,
				const any & p_separator,
				set_deep & p_deep
			) override;
		};

		struct type_ref :
			public type_link
		{
			type_ref(module_pointer p_module, t_integer & p_id) : type_link{p_module, p_id} {
				using namespace just::text_literals;
				m_is_local = true;
				name("$ref#"_jt);
			}

			void var_change(var_pointer p_to, any_const_pointer p_from) override;
		};

		// simple

		struct type_simple :
			public type_base
		{
			using type_base::type_base;

			void init_categories() override;
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
			type_null(module_pointer p_module, t_integer & p_id) : type_simple{p_module, p_id} {
				using namespace just::text_literals;
				name("$null#"_jt);
			}

			void variant_set(any_const_pointer p_any, t_variant & p_variant) override { p_variant = variant_null{}; }
		};

		struct type_all :
			public type_simple
		{
			type_all(module_pointer p_module, t_integer & p_id) : type_simple{p_module, p_id} {
				using namespace just::text_literals;
				name("$all#"_jt);
			}

			void variant_set(any_const_pointer p_any, t_variant & p_variant) override { p_variant = variant_all{}; }
		};

		struct type_category :
			public type_simple
		{
			type_category(module_pointer p_module, t_integer & p_id) : type_simple{p_module, p_id} {
				name("$category#"_jt);
			}

			bool write(
				output_pointer p_out,
				any_const_pointer p_any,
				const any & p_separator,
				set_deep & p_deep
			) override;
			void variant_set(any_const_pointer p_any, t_variant & p_variant) override;
		};

		struct type_type :
			public type_simple
		{
			type_type(module_pointer p_module, t_integer & p_id) : type_simple{p_module, p_id} {
				name("$type#"_jt);
			}

			bool write(
				output_pointer p_out,
				any_const_pointer p_any,
				const any & p_separator,
				set_deep & p_deep
			) override;
			auto element(any_const_pointer p_any, any_const_pointer p_key, bool & p_wrong_key) -> var_pointer override;
			var_pointer element_const(any_const_pointer p_any, const t_text_value & p_key, bool & p_wrong_key) override;
			void variant_set(any_const_pointer p_any, t_variant & p_variant) override;
		};

		struct type_bool :
			public type_simple
		{
			type_bool(module_pointer p_module, t_integer & p_id) : type_simple{p_module, p_id} {
				using namespace just::text_literals;
				name("$bool#"_jt);
			}

			bool write(
				output_pointer p_out,
				any_const_pointer p_any,
				const any & p_separator,
				set_deep & p_deep
			) override;
			void variant_set(any_const_pointer p_any, t_variant & p_variant) override;
		};

		struct type_simple_number :
			public type_simple
		{
			using type_simple::type_simple;

			void init_categories() override;
		};

		struct type_int :
			public type_simple_number
		{
			using t_limits = std::numeric_limits<t_integer>;

			type_int(module_pointer p_module, t_integer & p_id) : type_simple_number{p_module, p_id} {
				using namespace just::text_literals;
				name("$int#"_jt);
			}

			void init_target() override;
			bool write(
				output_pointer p_out,
				any_const_pointer p_any,
				const any & p_separator,
				set_deep & p_deep
			) override;
			void variant_set(any_const_pointer p_any, t_variant & p_variant) override;
		};

		struct type_float :
			public type_simple_number
		{
			using t_limits = std::numeric_limits<t_floating>;

			static constexpr t_floating s_infinity			= t_limits::infinity();
			static constexpr t_floating s_infinity_negative	= -t_limits::infinity();
			static constexpr t_floating s_nan				= t_limits::quiet_NaN();

			type_float(module_pointer p_module, t_integer & p_id) : type_simple_number{p_module, p_id} {
				using namespace just::text_literals;
				name("$float#"_jt);
			}

			void init_target() override;
			bool write(
				output_pointer p_out,
				any_const_pointer p_any,
				const any & p_separator,
				set_deep & p_deep
			) override;
			void variant_set(any_const_pointer p_any, t_variant & p_variant) override;
		};

		// compound

		struct type_compound :
			public type_base
		{
			type_compound(module_pointer p_module, t_integer & p_id) : type_base{p_module, p_id} {
				m_is_compound = true;
			}

			void init_categories() override;
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
			type_text(module_pointer p_module, t_integer & p_id) : type_compound{p_module, p_id} {
				using namespace just::text_literals;
				name("$text#"_jt);
			}

			bool write(
				output_pointer p_out,
				any_const_pointer p_any,
				const any & p_separator,
				set_deep & p_deep
			) override;
			var_pointer element_const(any_const_pointer p_any, const t_text_value & p_key, bool & p_wrong_key) override;
			void variant_set(any_const_pointer p_any, t_variant & p_variant) override;
		};

		struct type_array :
			public type_compound
		{
			type_array(module_pointer p_module, t_integer & p_id) : type_compound{p_module, p_id} {
				using namespace just::text_literals;
				name("$array#"_jt);
			}

			bool write(
				output_pointer p_out,
				any_const_pointer p_any,
				const any & p_separator,
				set_deep & p_deep
			) override;
			auto element(any_const_pointer p_any, any_const_pointer p_key, bool & p_wrong_key) -> var_pointer override;
			var_pointer element_const(any_const_pointer p_any, const t_text_value & p_key, bool & p_wrong_key) override;
			void variant_set(any_const_pointer p_any, t_variant & p_variant) override;
		};

		struct type_map :
			public type_compound
		{
			type_map(module_pointer p_module, t_integer & p_id) : type_compound{p_module, p_id} {
				name("$map#"_jt);
			}

			bool write(
				output_pointer p_out,
				any_const_pointer p_any,
				const any & p_separator,
				set_deep & p_deep
			) override;
			auto element(any_const_pointer p_any, any_const_pointer p_key, bool & p_wrong_key) -> var_pointer override;
			var_pointer element_const(any_const_pointer p_any, const t_text_value & p_key, bool & p_wrong_key) override;
			void variant_set(any_const_pointer p_any, t_variant & p_variant) override;
		};

		struct type_struct;
		using type_struct_pointer = type_struct *;

		// any

		union any_value {
			// data
			t_integer			m_int;
			t_floating			m_float;
			bool				m_bool;
			category::pointer	m_category;
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
			any();							// $null#
			any(variant_null);				// $null#
			any(variant_all);				// $all#
			any(bool p_value);				// $bool#
			any(t_integer p_value);			// $int#
			any(t_floating p_value);		// $float#
			any(type_pointer p_value);		// $type#
			any(category::pointer p_value);	// $category#

			auto any_get() -> any_pointer { return m_type->any_get(this); }
			auto any_get_const() const -> any_const_pointer { return m_type->any_get_const(this); }
			auto type_get() const -> type_pointer { return any_get_const()->m_type; }
			void close() { m_type->any_close(this); }
			bool write(output_pointer p_out = &just::g_console, const any & p_separator = any{}) const {
				set_deep v_set_compound;
				return m_type->write(p_out, this, p_separator, v_set_compound);
			}
			bool write(output_pointer p_out, const any & p_separator, set_deep & p_deep) const {
				return m_type->write(p_out, this, p_separator, p_deep);
			}

			var_pointer element(const t_text_value & p_key, bool & p_wrong_key) const;
			var_pointer element(const any & p_key, bool & p_wrong_key) const {
				return m_type->element(any_get_const(), p_key.any_get_const(), p_wrong_key);
			}
			var_pointer element_const(const t_text_value & p_key, bool & p_wrong_key) const {
				return m_type->element_const(any_get_const(), p_key, p_wrong_key);
			}

			void variant_set(t_variant & p_variant) const { m_type->variant_set(any_get_const(), p_variant); }
		};

		inline just::output_base & operator << (just::output_base & p_out, any_const_pointer p_value) {
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
			any_var(type_struct_pointer p_type); // struct

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
				return m_owners.m_next->node_target()->m_owner;
			}
		};

		using link_node = just::node_list<any_link>;
		using link_node_pointer = link_node *;

		//

		struct property_info {
			any_var			m_value;
			type_pointer	m_type_source;
		};

		struct extender {
			using pointer = extender *;
			using t_bases = just::map<type_pointer, pointer>;
			using t_bases_iter = t_bases::t_node::pointer;

			// data
			t_bases		m_bases;

			virtual type_pointer type() = 0;
		};

		struct type_struct :
			public type_compound,
			public extender,
			public just::node_list<type_struct>,
			public just::bases::with_deleter<type_struct *>
		{
			using t_props = just::hive<t_text_value, property_info, just::text_less>;

			// data
			t_props		m_props;

			type_struct(
				module_pointer p_module,
				t_integer & p_id,
				const creation_args & p_args
			) : type_compound{p_module, p_id}
			{
				m_is_struct = true;
				args_set(p_args);
			}

			type_pointer type() override { return this; }

			bool prop_add(
				const t_text_value & p_prop_name,
				const any_var & p_default = any_var{},
				type_pointer p_type_source = nullptr
			) {
				if( p_type_source == nullptr ) { p_type_source = this; }
				typename t_props::t_find_result v_res = m_props.maybe_emplace(p_prop_name, p_default, p_type_source);
				return v_res.m_added;
			}

			t_integer props_count() { return m_props.count(); }

			t_index inherit_from(const log_pos & p_log_pos, type_pointer p_type_source, log_pointer p_log) {
				if( !p_type_source->m_is_struct ) {
					p_log->add(p_log_pos.message(just::implode<t_char>(
						{"deduce error: Not struct type in extends: "sv, p_type_source->m_name_full}
					) ) );
					return 1;
				}
				if( p_type_source == this ) {
					p_log->add(p_log_pos.message(just::implode<t_char>(
						{"deduce error: Struct type can not extend itself: "sv, p_type_source->m_name_full}
					) ) );
					return 1;
				}
				if( m_bases.find(p_type_source) ) {
					p_log->add(p_log_pos.message(just::implode<t_char>(
						{"deduce error: Base type in extends is listed more than once: "sv, p_type_source->m_name_full}
					) ) );
					return 1;
				}
				type_struct_pointer v_type_source = static_cast<type_struct_pointer>(p_type_source);
				m_bases.maybe_emplace(p_type_source, v_type_source);
				t_index v_ret = 0;
				for( typename t_props::t_info && v_it : v_type_source->m_props ) {
					if( ! prop_add(v_it.key(), v_it.m_value->m_value, p_type_source) ) {
						typename t_props::t_find_result v_res = m_props.find(v_it.key() );
						type_pointer v_type = v_res.m_value->m_type_source;
						p_log->add(p_log_pos.message(just::implode<t_char>({
							"deduce error: Property \""sv, v_it.key(),
							"\" defined in type \""sv, p_type_source->m_name_full,
							"\" is already inherited from type: "sv,
							v_type->m_name_full
						}) ) );
						p_log->add(v_type->m_log_pos.message(just::implode<t_char>(
							{"info: See definition of type: "sv, v_type->m_name_full}
						) ) );
						p_log->add(v_type_source->m_log_pos.message(just::implode<t_char>(
							{"info: See definition of type: "sv, v_type_source->m_name_full}
						) ) );
						++v_ret;
					}
				}
				return v_ret;
			}

			void init_categories() override;
			auto element(any_const_pointer p_any, any_const_pointer p_key, bool & p_wrong_key) -> var_pointer override;
			var_pointer element_const(any_const_pointer p_any, const t_text_value & p_key, bool & p_wrong_key) override;
			void variant_set(any_const_pointer p_any, t_variant & p_variant) override;
		};

		// compound_base

		struct compound_base :
			public just::bases::with_deleter<compound_base *>
		{
			using t_node = just::node_list<any_link>;

			// data
			t_node	m_links_strong;
			t_node	m_links_weak;

			virtual ~compound_base() = default;

			compound_text_pointer	get_text();
			compound_array_pointer	get_array();
			compound_map_pointer	get_map();
			compound_struct_pointer	get_struct();

			bool link_is_primary(link_pointer v_link) { return m_links_strong.m_next == v_link; }

			link_pointer link_get_primary() {
				return m_links_strong.node_empty() ? nullptr :
					m_links_strong.m_next->node_target()
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

		// compound_with_lock

		struct compound_with_lock :
			public compound_base
		{
			// data
			t_int_ptr	m_lock;

			void lock_add() { ++m_lock; }
			void lock_del() { --m_lock; }
			bool lock_check() { return m_lock; }
		};

		// compound_array

		struct compound_array :
			public compound_with_lock
		{
			using t_items = just::array_alias<any_var, just::capacity_step<8, 8> >;

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

		// compound_map

		struct any_less {
			bool operator () (const any & p1, const any & p2) const;
		};

		struct compound_map :
			public compound_with_lock
		{
			using t_items = just::map<any_var, any_var, any_less>;

			// data
			t_items		m_items;
			any_var		m_count;

			compound_map() = default;

			t_integer count() { return m_items.count(); }
		};

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
				t_integer v_count = p_type->props_count();
				if( v_count ) {
					{
						t_items v_items(v_count);
						std::ranges::swap(m_items, v_items);
					}
					for( t_integer v_index = 0; v_index < v_count; ++v_index ) {
						m_items[v_index].var_owner_set(this);
						m_items[v_index] = p_type->m_props.m_vector[v_index].m_value;
					}
				}
			}

			t_integer count() { return m_items.size(); }
		};

		inline compound_text_pointer compound_base::get_text() { return static_cast<compound_text_pointer>(this); }
		inline compound_array_pointer compound_base::get_array() { return static_cast<compound_array_pointer>(this); }
		inline compound_map_pointer compound_base::get_map() { return static_cast<compound_map_pointer>(this); }
		inline compound_struct_pointer compound_base::get_struct() { return static_cast<compound_struct_pointer>(this); }

		// static_data

		struct static_data :
			public static_data_base
		{
			// data
			t_integer		m_id_props;
			t_integer		m_id_consts;
			type_struct		m_struct_props;
			type_struct		m_struct_consts;
			any_var			m_props;
			any_var			m_consts;

			static_data(const t_text_value & p_type_name, t_integer p_id);

			void init() override {
				m_props = any_var(&m_struct_props);
				m_consts = any_var(&m_struct_consts);
			}
		};

		// init()

		inline void type_base::init_start() {
			m_static = std::make_unique<static_data>(m_name, m_id);
		}

		inline static_data_pointer type_base::get_static() {
			return static_cast<static_data_pointer>(m_static.get() );
		}

		void type_int::init_target() {
			using namespace just::text_literals;
			static_data_pointer v_static = get_static();
			v_static->m_struct_consts.prop_add("min#"_jt, t_limits::min() );
			v_static->m_struct_consts.prop_add("max#"_jt, t_limits::max() );
		}

		void type_float::init_target() {
			using namespace just::text_literals;
			static_data_pointer v_static = get_static();
			v_static->m_struct_consts.prop_add("min#"_jt,				t_limits::min() );
			v_static->m_struct_consts.prop_add("max#"_jt,				t_limits::max() );
			v_static->m_struct_consts.prop_add("infinity#"_jt,			t_limits::infinity() );
			v_static->m_struct_consts.prop_add("infinity_negative#"_jt,	-t_limits::infinity() );
			v_static->m_struct_consts.prop_add("nan#"_jt,				t_limits::quiet_NaN() );
			v_static->m_struct_consts.prop_add("epsilon#"_jt,			t_limits::epsilon() );
		}

		// element()

		inline var_pointer any::element(const t_text_value & p_key, bool & p_wrong_key) const {
			any_var v_key{p_key};
			return element(v_key, p_wrong_key);
		}

	} // ns

} // ns