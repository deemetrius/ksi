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

		struct body {

			struct node_tree;
			using node_tree_pointer = node_tree *;
			using body_pointer = body *;

			//using fn_perform = void (*) (body_pointer p_body, node_tree_pointer p_parent, node_tree_pointer p_node);
			static void do_root(body_pointer p_body, node_tree_pointer p_parent, node_tree_pointer p_node);
			static void do_leaf(body_pointer p_body, node_tree_pointer p_parent, node_tree_pointer p_node);
			static void do_normal_left_right(body_pointer p_body, node_tree_pointer p_parent, node_tree_pointer p_node);

			using fn_perform = decltype(&do_root);

			struct node_type {
				using const_pointer = const node_type *;

				// data
				fn_perform	m_perform;
				precedence	m_before;
				precedence	m_after;
			};

			static constexpr node_type s_type_root{&do_root, prec_root, prec_root};
			static constexpr node_type s_type_leaf{&do_leaf, prec_leaf, prec_leaf};

			struct node_tree :
				public just::node_list<node_tree>
			{
				using t_node = just::node_list<node_tree>;

				// data
				node_type::const_pointer	m_type;
				node_tree_pointer			m_left = nullptr;
				node_tree_pointer			m_right = nullptr;
				instr						m_instr;

				node_tree(node_type::const_pointer p_type, instr_type::const_pointer p_instr_type) :
					m_type{p_type}, m_instr{p_instr_type}
				{}
			};

			struct tree :
				public just::node_list<tree>
			{
				using pointer = tree *;
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

			struct scope :
				public just::node_list<scope>
			{
				using pointer = scope *;
				using t_list = just::list<tree, just::closers::simple_none>;

				// data
				t_list	m_trees;

				scope(body_pointer p_body) {
					tree::pointer v_tree = p_body->m_pool_tree.emplace(p_body);
					m_trees.append(v_tree);
				}
			};

			using t_scopes = just::list<scope>;

			// data
			function_body_user::pointer	m_fn;
			just::pool<node_tree, 16>	m_pool_node;
			just::pool<tree, 16>		m_pool_tree;
			just::pool<scope, 16>		m_pool_scope;
			t_scopes					m_scopes;

			body(function_body_user::pointer p_fn) : m_fn{p_fn} {
				m_scopes.append( m_pool_scope.emplace(this) );
			}

		}; // struct body

		void body::do_root(body_pointer p_body, node_tree_pointer p_parent, node_tree_pointer p_node) {
			node_tree_pointer v_node = p_node->m_right;
			if( v_node ) { v_node->m_type->m_perform(p_body, p_node, v_node); }
		}

		void body::do_leaf(body_pointer p_body, node_tree_pointer p_parent, node_tree_pointer p_node) {
			// here add instr
		}

		void body::do_normal_left_right(body_pointer p_body, node_tree_pointer p_parent, node_tree_pointer p_node) {
			node_tree_pointer v_node = p_node->m_left;
			v_node->m_type->m_perform(p_body, p_node, v_node);
			v_node = p_node->m_right;
			v_node->m_type->m_perform(p_body, p_node, v_node);
			// here add instr
		}

	} // ns

} // ns