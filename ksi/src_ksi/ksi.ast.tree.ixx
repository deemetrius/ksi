module;

#include "../src/pre.h"

export module ksi.ast:tree;

export import just.pool;
export import ksi.act;

export namespace ksi {

	namespace ast {

		enum class precedence {
			n_leaf,
			n_member,			//	a.key a[index]
			n_assign_before,	//	a=
			n_mult,				//	* / .mod
			n_plus,				//	+ -
			n_cmp,				//	== <> < > <= >=
			n_assign_after,		//	=a
			n_root
		};

		enum class kind { n_none };

		struct body {

			using body_pointer = body *;

			struct node;
			using node_pointer = node *;

			struct tree;
			using tree_pointer = tree *;

			using fn_perform = void (*) (body_pointer p_body, node_pointer p_parent, node_pointer p_node);

			struct node_type {
				using const_pointer = const node_type *;
				using fn_add = void (*) (tree_pointer p_tree, node_pointer p_node);

				// data
				fn_perform
					m_perform;
				fn_add
					m_fn_add;
				precedence
					m_prec_before,
					m_prec_after;
				kind
					m_kind = kind::n_none;

				void node_add(body_pointer p_body, tree_pointer p_tree,
					act::action::t_cref p_action, tree_pointer p_other_tree = nullptr
				) const {
					node_pointer v_node = p_body->m_pool_nodes.make(this, p_action, p_other_tree);
					m_fn_add(p_tree, v_node);
				}
			};

			struct node {
				// data
				node_type::const_pointer
					m_type;
				act::action
					m_action;
				tree_pointer
					m_tree = nullptr;
				node_pointer
					m_left = nullptr,
					m_right = nullptr;
			};

			struct tree {
				// data
				tree_pointer
					m_sibling = nullptr;
				node_pointer
					m_root,
					m_last;

				tree(body_pointer p_body) {
					node_types::s_root.node_add(p_body, this, {&act::actions::s_nothing});
				}
			};

			struct node_types {
				static void do_root(body_pointer p_body, node_pointer p_parent, node_pointer p_node) {
					node_pointer v_node = p_node->m_right;
					if( v_node ) { v_node->m_type->m_perform(p_body, p_node, v_node); }
				}

				static void do_leaf(body_pointer p_body, node_pointer p_parent, node_pointer p_node) {
					p_body->action_add(p_node->m_action);
				}

				static void do_usual_left_right(body_pointer p_body, node_pointer p_parent, node_pointer p_node) {
					node_pointer v_node = p_node->m_left;
					v_node->m_type->m_perform(p_body, p_node, v_node);
					v_node = p_node->m_right;
					v_node->m_type->m_perform(p_body, p_node, v_node);
					p_body->action_add(p_node->m_action);
				}

				static void do_usual_right_left(body_pointer p_body, node_pointer p_parent, node_pointer p_node) {
					node_pointer v_node = p_node->m_right;
					v_node->m_type->m_perform(p_body, p_node, v_node);
					v_node = p_node->m_left;
					v_node->m_type->m_perform(p_body, p_node, v_node);
					p_body->action_add(p_node->m_action);
				}

				static void add_root(tree_pointer p_tree, node_pointer p_node) {
					p_tree->m_root = p_node;
					p_tree->m_last = p_node;
				}

				static inline const node_type
					s_root{&do_root, &add_root, precedence::n_root, precedence::n_root}
				;
			}; // types

			using t_trees = std::vector<tree_pointer>;
			using t_groups = std::vector<t_index>;

			// data
			act::sequence::pointer
				m_seq;
			just::pool<node, 16>
				m_pool_nodes;
			just::pool<tree, 16>
				m_pool_trees;
			t_trees
				m_trees;
			tree_pointer
				m_main_tree;
			t_groups
				m_groups;

			body(act::sequence::pointer p_seq) : m_seq{p_seq} {
				m_main_tree = tree_add_group();
				m_groups.emplace_back(0);
			}

			// trees

			tree_pointer tree_current() {
				return m_trees.back();
			}

			tree_pointer tree_add_sibling() {
				tree_pointer ret = m_pool_trees.make(this);
				m_trees.back() = ret;
				return ret;
			}

			tree_pointer tree_add_group() {
				tree_pointer ret = m_pool_trees.make(this);
				m_trees.emplace_back(ret);
				return ret;
			}

			void tree_pop() {
				m_trees.pop_back();
			}

			// groups

			act::action_group::pointer group_current() {
				return &m_seq->m_groups[m_groups.back()];
			}

			void group_push() {
				m_groups.emplace_back( m_groups.size() );
				m_seq->m_groups.emplace_back();
			}

			void group_pop() {
				m_groups.pop_back();
			}

			// actions

			void action_add(act::action::t_cref p_action) {
				group_current()->m_actions.emplace_back(p_action);
			}

		}; // body

	} // ns

} // ns