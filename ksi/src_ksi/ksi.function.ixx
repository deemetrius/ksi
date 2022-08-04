module;

#include "../src/pre.h"

export module ksi.function;

export import <vector>;
export import just.text;
export import just.array;
export import just.hive;
export import ksi.var;

export namespace ksi {
	
	using namespace just::text_literals;
	using namespace std::literals::string_view_literals;

	struct space;
	struct function_body;

	using function_body_pointer = function_body *;

	struct call_space {
		using pointer = call_space *;
		using t_args = std::vector<var::any_var>;

		// data
		function_body_pointer	m_body;
		t_args					m_args;
		t_args					m_vars;

		call_space(function_body_pointer p_body);
	};

	struct stack {
		using t_items = just::array_alias<var::any_var, just::capacity_step<16, 16> >;

		// data
		t_items				m_items;
		//var::var_pointer	m_var;

		var::any_var * last() { return m_items.end() -1; }

		void var_add(const var::any_var & p_var) {
			just::array_append(m_items, p_var);
		}
		
		void var_link(var::var_pointer p_var) {
			just::array_append(m_items);
			last()->link_to(p_var);
		}

		void var_ref(var::var_pointer p_var) {
			just::array_append(m_items);
			last()->ref_to(p_var);
		}

		void var_remove(t_index p_amount) {
			just::array_remove_from_end(m_items, p_amount);
		}

		/*void var_set(var::var_pointer p_var) {
			m_var = p_var;
		}

		void var_put() {
			just::array_append(m_items, *m_var);
		}

		void var_put_link() {
			just::array_append(m_items);
			last()->link_to(m_var);
		}

		void var_put_ref() {
			just::array_append(m_items);
			last()->ref_to(m_var);
		}*/
	};

	//

	struct instr_data {
		using const_reference = const instr_data &;

		// data
		position	m_pos;
		t_integer	m_arg = 0;
		t_index		m_param = 0;
		t_index		m_group = 0;
	};

	struct instr_type {
		static void do_nothing(space * p_space, call_space * p_call, stack * p_stack,
			log_base::pointer p_log, instr_data::const_reference p_data
		) {}

		static const instr_type s_nothing;

		using t_fn = decltype(&do_nothing);
		using t_text = const t_text_value::t_impl_base *;
		using const_pointer = const instr_type *;

		// data
		t_text	m_name;
		t_fn	m_fn = &do_nothing;

		bool empty() const { return m_fn == &do_nothing; }
	};

	const instr_type instr_type::s_nothing{"do_nothing"_jt, &instr_type::do_nothing};

	struct instr {
		// data
		instr_type::const_pointer	m_type = &instr_type::s_nothing;
		instr_data					m_data;

		bool empty() const { return m_type->empty(); }
	};

	struct instr_group {
		using pointer = instr_group *;
		using t_instructions = std::vector<instr>;

		// data
		t_instructions	m_instructions;
	};

	//

	struct function_body {
		using pointer = function_body *;
		using t_args = just::hive<t_text_value, var::any_var, just::text_less>;
		using t_args_insert = t_args::t_find_result;
		using t_vars = just::hive<t_text_value, std::monostate, just::text_less>;
		using t_vars_insert = t_vars::t_find_result;

		// data
		module_pointer	m_module;
		log_pos			m_log_pos;
		t_args			m_args;
		t_vars			m_vars;

		function_body(module_pointer p_module, log_pos p_log_pos) : m_module{p_module}, m_log_pos{p_log_pos} {
			m_vars.maybe_emplace("ret"_jt);
		}

		bool arg_add(const t_text_value & p_name, const var::any_var & p_value = var::any_var{}) {
			t_args_insert v_res = m_args.maybe_emplace(p_name, p_value);
			return v_res.m_added;
		}

		t_index var_id(const t_text_value & p_name) {
			t_vars_insert v_res = m_vars.maybe_emplace(p_name);
			return v_res.m_index;
		}
	};

	struct function_body_user :
		public function_body,
		public just::node_list<function_body_user>,
		public just::bases::with_deleter<function_body_user *>
	{
		using pointer = function_body_user *;
		using t_groups = std::vector<instr_group>;

		// data
		t_groups	m_groups;

		function_body_user(module_pointer p_module, const log_pos & p_log_pos) :
			function_body{p_module, p_log_pos}
		{}

		void write(output_pointer p_out) {
			t_size v_group_pos = 0;
			for( instr_group & v_group : m_groups ) {
				(*p_out) << "group: " << v_group_pos << just::g_new_line;
				for( instr & v_instr : v_group.m_instructions ) {
					(*p_out) << v_instr.m_type->m_name->m_text << ":\t" << v_instr.m_data.m_arg << '\t'
						<< v_instr.m_data.m_param << '\t' << v_instr.m_data.m_group << '\n'
					;
				}
				++v_group_pos;
			}
		}
	};

	struct function :
		public var::with_name,
		public just::node_list<function>,
		public just::bases::with_deleter<function *>
	{
		using pointer = function *;
		using t_over_category = std::map<var::category::pointer, function_body::pointer, std::ranges::less>;
		using t_over_category_insert = std::pair<t_over_category::iterator, bool>;
		using t_over_type = std::map<var::type_pointer, function_body::pointer, std::ranges::less>;
		using t_over_type_insert = std::pair<t_over_type::iterator, bool>;

		// data
		function_body::pointer		m_common = nullptr;
		t_over_type					m_by_type;
		t_over_category				m_by_category;

		function(module_pointer	p_module, const t_text_value & p_name) : with_name{p_module} { name(p_name, true); }

		bool overload(var::category::pointer p_key, function_body::pointer p_body,
			bool p_is_global, log_base::pointer p_log
		) {
			t_over_category_insert v_res = m_by_category.try_emplace(p_key, p_body);
			if( !v_res.second ) {
				t_text_value v_fn_name = m_name_full;
				if( p_is_global ) { v_fn_name = m_name; }
				t_text_value p_message = just::implode<t_char>(
					{"notice: Function "sv, v_fn_name, " was already overloaded for category: "sv, p_key->m_name_full}
				);
				p_log->add(p_body->m_log_pos.message(p_message) );
			}
			return v_res.second;
		}

		bool overload(var::type_pointer p_key, function_body::pointer p_body,
			bool p_is_global, log_base::pointer p_log
		) {
			t_over_type_insert v_res = m_by_type.try_emplace(p_key, p_body);
			if( !v_res.second ) {
				t_text_value v_fn_name = m_name_full;
				if( p_is_global ) { v_fn_name = m_name; }
				t_text_value p_message = just::implode<t_char>(
					{"notice: Function "sv, v_fn_name, " was already overloaded for type: "sv, p_key->m_name_full}
				);
				p_log->add(p_body->m_log_pos.message(p_message) );
			}
			return v_res.second;
		}

		bool overload(std::monostate p_key, function_body::pointer p_body,
			bool p_is_global, log_base::pointer p_log
		) {
			//just::g_console << "{here: " << m_name_full << " }\n";
			bool ret = false;
			if( m_common ) {
				t_text_value v_fn_name = m_name_full;
				if( p_is_global ) { v_fn_name = m_name; }
				t_text_value p_message = just::implode<t_char>(
					{"notice: Function "sv, v_fn_name, " was already overloaded in common way."sv}
				);
				p_log->add(p_body->m_log_pos.message(p_message) );
			} else {
				ret = true;
				m_common = p_body;
			}
			return ret;
		}
	};

	//

	call_space::call_space(function_body_pointer p_body) :
		m_body{p_body},
		m_args{p_body->m_args.count()},
		m_vars{p_body->m_vars.count()}
	{}

} // ns