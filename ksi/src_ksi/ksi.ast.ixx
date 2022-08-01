module;

#include "../src/pre.h"

export module ksi.ast;

export import just.pool;
export import ksi.config;
import ksi.instructions;

export namespace ksi {

	namespace ast {

		enum precedence {
			prec_leaf,
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
			using fn_add = void (*) (tree_pointer p_tree, node_tree_pointer p_node);

			struct node_type {
				using const_pointer = const node_type *;

				// data
				fn_perform	m_perform;
				fn_add		m_add;
				precedence	m_before;
				precedence	m_after;
				kind		m_kind = kind::none;
			};

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
			};

			struct tree :
				public just::node_list<tree>
			{
				//using pointer = tree *;
				using t_node = just::node_list<tree>;
				using t_list = just::list<node_tree, just::closers::simple_none>;

				// data
				t_list	m_nodes;

				tree(body_pointer p_body) {
					node_tree_pointer v_node = p_body->m_pool_node.emplace(&s_type_root, &instructions::g_nothing);
					m_nodes.append(v_node);
				}

				node_tree_pointer root() { return m_nodes.first()->node_target(); }
				node_tree_pointer last() { return m_nodes.last()->node_target(); }
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

			static void node_add_inner(tree_pointer p_tree, node_tree_pointer p_target, node_tree_pointer p_node) {
				p_tree->m_nodes.append(p_node);
				p_node->m_left = p_target->m_right;
				p_target->m_right = p_node;
			}

			// left-to-right
			static void node_add_assoc_left(tree_pointer p_tree, node_tree_pointer p_node) {
				node_tree_pointer v_target = p_tree->root();
				while( v_target->m_right->m_type->m_before > p_node->m_type->m_after ) {
					v_target = v_target->m_right;
				}
				node_add_inner(p_tree, v_target, p_node);
			}

			// right-to-left
			static void node_add_assoc_right(tree_pointer p_tree, node_tree_pointer p_node) {
				node_tree_pointer v_target = p_tree->root();
				while( v_target->m_right->m_type->m_before >= p_node->m_type->m_after ) {
					v_target = v_target->m_right;
				}
				node_add_inner(p_tree, v_target, p_node);
			}

			// root
			static void node_add_root(tree_pointer p_tree, node_tree_pointer p_node) {
				p_tree->m_nodes.append(p_node);
			}

			// leaf
			static void node_add_leaf(tree_pointer p_tree, node_tree_pointer p_node) {
				p_tree->last()->m_right = p_node;
			}

			//

			static constexpr node_type s_type_root{&do_root, &node_add_root, prec_root, prec_root};
			static constexpr node_type s_type_leaf{&do_leaf, &node_add_leaf, prec_leaf, prec_leaf};

			struct scope :
				public just::node_list<scope>
			{
				//using pointer = scope *;
				using t_list = just::list<tree, just::closers::simple_none>;

				// data
				instr	m_instr_step, m_instr_last;
				t_list	m_trees;

				scope(body_pointer p_body) {
					tree_pointer v_tree = p_body->m_pool_tree.emplace(p_body);
					m_trees.append(v_tree);
				}

				tree_pointer tree_last() { return m_trees.last()->node_target(); }
			};

			using t_scopes = just::list<scope, just::closers::simple_none>;

			// data
			function_body_user::pointer		m_fn;
			just::pool<node_tree, 16>		m_pool_node;
			just::pool<tree, 16>			m_pool_tree;
			just::pool<scope, 16>			m_pool_scope;
			t_scopes						m_scopes;
			std::vector<t_size>				m_group_pos;

			body(function_body_user::pointer p_fn) : m_fn{p_fn} {
				m_scopes.append( m_pool_scope.emplace(this) );
				group_push();
			}

			scope_pointer scope_last() { return m_scopes.last()->node_target(); }
			tree_pointer tree_last() { return scope_last()->tree_last(); }

			node_tree_pointer node_add(node_type::const_pointer p_node_type, instr_type::const_pointer p_instr_type,
				const instr_data & p_instr_data = instr_data{}, scope_pointer p_scope = nullptr
			) {
				node_tree_pointer v_node = m_pool_node.emplace(p_node_type, p_instr_type, p_instr_data, p_scope);
				v_node->m_type->m_add(tree_last(), v_node);
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
				// todo
			}

		}; // struct body

	} // ns

} // ns