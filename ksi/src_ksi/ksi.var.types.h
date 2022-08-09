
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
	virtual void from(any_var & p_to, any_var & p_from, bool & p_bad_conversion);
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
	void from(any_var & p_to, any_var & p_from, bool & p_bad_conversion) override;
};

struct type_all :
	public type_simple
{
	type_all(module_pointer p_module, t_integer & p_id) : type_simple{p_module, p_id} {
		using namespace just::text_literals;
		name("$all#"_jt);
	}

	void variant_set(any_const_pointer p_any, t_variant & p_variant) override { p_variant = variant_all{}; }
	void from(any_var & p_to, any_var & p_from, bool & p_bad_conversion) override;
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
	void from(any_var & p_to, any_var & p_from, bool & p_bad_conversion) override;
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
	void from(any_var & p_to, any_var & p_from, bool & p_bad_conversion) override;
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
	void from(any_var & p_to, any_var & p_from, bool & p_bad_conversion) override;
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
	void from(any_var & p_to, any_var & p_from, bool & p_bad_conversion) override;
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
	void from(any_var & p_to, any_var & p_from, bool & p_bad_conversion) override;
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
