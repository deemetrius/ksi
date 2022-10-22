module;

#include "../src/pre.h"

export module ksi.ast;

export import <span>;
export import just.pool;
export import ksi.config;
import ksi.instructions;

export namespace ksi { namespace ast {

	using namespace just::text_literals;

	enum precedence {
		prec_leaf,
		prec_multiply,						// * / mod		(numbers)
		prec_add,							// + -
		prec_bits_shift,					// shift		(bitwise)
		prec_bits_and,						// &			(bitwise)
		prec_bits_xor	= prec_bits_and,	// ^
		prec_bits_or	= prec_bits_and,	// |
		prec_root
	};

	enum class kind { none };

	struct body {

		struct node_tree;
		struct tree;
		struct scope;
		using node_tree_pointer = node_tree *;
		using tree_pointer = tree *;
		using scope_pointer = scope *;
		using body_pointer = body *;

		using fn_perform = void (*) (body_pointer p_body, node_tree_pointer p_parent, node_tree_pointer p_node);
		using fn_add = void (*) (body_pointer p_body, node_tree_pointer p_node);

		struct node_type {
			using const_pointer = const node_type *;

			// data
			fn_perform	m_perform;
			fn_add		m_add;
			precedence	m_before;
			precedence	m_after;
			kind		m_kind = kind::none;
		};

		static const node_type
		s_type_leaf,
		s_type_multiply,
		s_type_add,
		s_type_bits_shift,
		s_type_bits_and,
		s_type_bits_xor,
		s_type_bits_or,
		s_type_root;

		struct node_tree :
			public just::node_list<node_tree>
		{
			using t_node = just::node_list<node_tree>;

			// data
			node_type::const_pointer	m_type;
			node_tree_pointer			m_left = nullptr;
			node_tree_pointer			m_right = nullptr;
			instr						m_instr;
			scope_pointer				m_scope = nullptr;

			node_tree(node_type::const_pointer p_type, instr_type::const_pointer p_instr_type,
				const instr_data & p_instr_data = instr_data{}, scope_pointer p_scope = nullptr
			) :
				m_type{p_type}, m_instr{p_instr_type, p_instr_data}, m_scope{p_scope}
			{}

			bool empty() { return m_right == nullptr; }
		};

		struct tree :
			public just::node_list<tree>
		{
			//using pointer = tree *;
			//using t_node = just::node_list<tree>;
			using t_list = just::list<node_tree, just::closers::simple_none>;

			// data
			t_list	m_nodes;

			tree(body_pointer p_body) {
				node_tree_pointer v_node = p_body->m_pool_node.emplace(&s_type_root, &instr_type::s_nothing);
				m_nodes.append(v_node);
			}

			node_tree_pointer root() { return m_nodes.first()->node_target(); }
			node_tree_pointer last() { return m_nodes.last()->node_target(); }

			bool empty() { return root()->empty(); }

			void apply(body_pointer p_body) {
				node_tree_pointer v_node = root();
				v_node->m_type->m_perform(p_body, v_node, v_node);
			}
		};

		//

		static void do_root(body_pointer p_body, node_tree_pointer p_parent, node_tree_pointer p_node) {
			node_tree_pointer v_node = p_node->m_right;
			if( v_node ) { v_node->m_type->m_perform(p_body, p_node, v_node); }
		}

		static void do_leaf(body_pointer p_body, node_tree_pointer p_parent, node_tree_pointer p_node) {
			p_body->instr_add(p_node->m_instr);
		}

		static void do_normal_left_right(body_pointer p_body, node_tree_pointer p_parent, node_tree_pointer p_node) {
			node_tree_pointer v_node = p_node->m_left;
			v_node->m_type->m_perform(p_body, p_node, v_node);
			v_node = p_node->m_right;
			v_node->m_type->m_perform(p_body, p_node, v_node);
			p_body->instr_add(p_node->m_instr);
		}

		static void do_normal_right_left(body_pointer p_body, node_tree_pointer p_parent, node_tree_pointer p_node) {
			node_tree_pointer v_node = p_node->m_right;
			v_node->m_type->m_perform(p_body, p_node, v_node);
			v_node = p_node->m_left;
			v_node->m_type->m_perform(p_body, p_node, v_node);
			p_body->instr_add(p_node->m_instr);
		}

		//

		struct scope :
			public just::node_list<scope>
		{
			using t_list = just::list<tree, just::closers::simple_none>;

			// data
			instr	m_instr_step, m_instr_last;
			t_list	m_trees;

			scope(body_pointer p_body) {
				tree_add(p_body);
			}

			tree_pointer tree_add(body_pointer p_body) {
				tree_pointer v_tree = p_body->m_pool_tree.emplace(p_body);
				m_trees.append(v_tree);
				return v_tree;
			}

			tree_pointer tree_last() { return m_trees.last()->node_target(); }

			void apply(body_pointer p_body) {
				tree_pointer v_tree_last = tree_last();
				for( tree_pointer v_it : m_trees ) {
					v_it->apply(p_body);
					if( ! v_it->empty() ) {
						if( v_it == v_tree_last ) { p_body->instr_add(m_instr_last); }
						else { p_body->instr_add(m_instr_step); }
					}
				}
			}
		};

		//

		static void node_add_inner(tree_pointer p_tree, node_tree_pointer p_target, node_tree_pointer p_node) {
			p_tree->m_nodes.append(p_node);
			p_node->m_left = p_target->m_right;
			p_target->m_right = p_node;
		}

		// left-to-right
		static void node_add_assoc_left(body_pointer p_body, node_tree_pointer p_node) {
			tree_pointer v_tree = p_body->tree_last();
			node_tree_pointer v_target = v_tree->root();
			while( v_target->m_right->m_type->m_before > p_node->m_type->m_after ) {
				v_target = v_target->m_right;
			}
			node_add_inner(v_tree, v_target, p_node);
		}

		// right-to-left
		static void node_add_assoc_right(body_pointer p_body, node_tree_pointer p_node) {
			tree_pointer v_tree = p_body->tree_last();
			node_tree_pointer v_target = v_tree->root();
			while( v_target->m_right->m_type->m_before >= p_node->m_type->m_after ) {
				v_target = v_target->m_right;
			}
			node_add_inner(v_tree, v_target, p_node);
		}

		// root
		static void node_add_root(body_pointer p_body, node_tree_pointer p_node) {
			p_body->tree_last()->m_nodes.append(p_node);
		}

		// leaf
		static void node_add_leaf(body_pointer p_body, node_tree_pointer p_node) {
			node_tree_pointer v_node_target = p_body->tree_last()->last();
			if( ! v_node_target->empty() ) {
				tree_pointer v_tree = p_body->scope_last()->tree_add(p_body);
				v_node_target = v_tree->last();
			}
			v_node_target->m_right = p_node;
		}

		//

		using t_scopes = just::list<scope, just::closers::simple_none>;

		// data
		function_body_user::pointer		m_fn;
		just::pool<node_tree, 16>		m_pool_node;
		just::pool<tree, 16>			m_pool_tree;
		just::pool<scope, 16>			m_pool_scope;
		t_scopes						m_scopes;
		std::vector<t_size>				m_group_pos;

		body(function_body_user::pointer p_fn) : m_fn{p_fn} {
			scope_pointer v_scope = m_pool_scope.emplace(this);
			m_scopes.append(v_scope);
			v_scope->m_instr_step.m_type = &instructions::s_pop;
			v_scope->m_instr_last.m_type = &instructions::s_pop;
			group_push();
		}

		scope_pointer scope_first() { return m_scopes.first()->node_target(); }
		scope_pointer scope_last() { return m_scopes.last()->node_target(); }
		tree_pointer tree_last() { return scope_last()->tree_last(); }

		// node_add
		node_tree_pointer node_add(node_type::const_pointer p_node_type, instr_type::const_pointer p_instr_type,
			const instr_data & p_instr_data = instr_data{}, scope_pointer p_scope = nullptr
		) {
			node_tree_pointer v_node = m_pool_node.emplace(p_node_type, p_instr_type, p_instr_data, p_scope);
			v_node->m_type->m_add(this, v_node);
			return v_node;
		} 

		//

		void group_push() {
			m_group_pos.push_back( m_fn->m_groups.size() );
			m_fn->m_groups.emplace_back();
		}

		void group_pop() {
			m_group_pos.pop_back();
		}

		instr_group::pointer group_current() {
			return &m_fn->m_groups[m_group_pos.back()];
		}

		void instr_add(const instr & p_instr) {
			if( ! p_instr.empty() ) { group_current()->m_instructions.push_back(p_instr); }
		}

		//

		void apply() {
			scope_first()->apply(this);
		}

		struct operator_info {
			using pointer = operator_info *;
			using t_map = std::map<t_text_value, operator_info, just::text_less>;

			// data
			t_text_value					m_text;
			body::node_type::const_pointer	m_node_type;
			instr_type::const_pointer		m_instr_type;
			instr_data_only					m_instr_data;

			static std::span<operator_info> ops_common() {
				static operator_info v_inst[] = {
					{"+"_jt, &body::s_type_add,			&instructions::s_op_add},
					{"-"_jt, &body::s_type_add,			&instructions::s_op_subtract},
					{"*"_jt, &body::s_type_multiply,	&instructions::s_op_multiply},
					{"/"_jt, &body::s_type_multiply,	&instructions::s_op_divide},
					{"&"_jt, &body::s_type_bits_and,	&instructions::s_op_bits_and},
					{"^"_jt, &body::s_type_bits_xor,	&instructions::s_op_bits_xor},
					{"|"_jt, &body::s_type_bits_or,		&instructions::s_op_bits_or}
				};
				return v_inst;
			}

			static t_map & ops_named() {
				t_text_value v_name;
				static t_map v_inst{
					{v_name = "mod"_jt,		{v_name, &body::s_type_multiply, &instructions::s_op_modulo}},
					{v_name = "shift"_jt,	{v_name, &body::s_type_bits_shift, &instructions::s_op_bits_shift}}
				};
				return v_inst;
			}
		};

		// node_add_op
		node_tree_pointer node_add_op(operator_info::pointer p_info, const position & p_pos) {
			return node_add(p_info->m_node_type, p_info->m_instr_type, instr_data::make(p_pos, p_info->m_instr_data) );
		}

	}; // struct body

	const body::node_type
	body::s_type_leaf		{&body::do_leaf, &body::node_add_leaf, prec_leaf, prec_leaf},
	body::s_type_multiply	{&body::do_normal_left_right, &body::node_add_assoc_left, prec_multiply, prec_multiply},
	body::s_type_add		{&body::do_normal_left_right, &body::node_add_assoc_left, prec_add, prec_add},
	body::s_type_bits_shift	{&body::do_normal_left_right, &body::node_add_assoc_left, prec_bits_shift, prec_bits_shift},
	body::s_type_bits_and	{&body::do_normal_left_right, &body::node_add_assoc_left, prec_bits_and, prec_bits_and},
	body::s_type_bits_xor	{&body::do_normal_left_right, &body::node_add_assoc_left, prec_bits_xor, prec_bits_xor},
	body::s_type_bits_or	{&body::do_normal_left_right, &body::node_add_assoc_left, prec_bits_or, prec_bits_or},
	body::s_type_root		{&body::do_root, &body::node_add_root, prec_root, prec_root};

} } // ns ns