module;

#include "../src/pre.h"

export module ksi.act;

export import ksi.var;
export import ksi.log;
export import <vector>;
export import <memory>;

export namespace ksi {

	struct space;

	namespace act {

		using namespace std::string_literals;

		using space_pointer = space *;

		struct sequence;
		using sequence_pointer = sequence *;

		struct stack {
			using t_items = var::optr_nest::o_vector<var::value>;

			// data
			var::junction
				m_point;
			var::cell
				m_empty_cell,
				m_cell;
			t_items
				m_items;

			stack() : m_empty_cell{&m_point}, m_cell{&m_point}, m_items{16, &m_point} {}

			var::cell & last() { return m_items.back(); }

			var::cell & pre_last() {
				t_index v_index = m_items.ssize() - 2;
				return m_items[v_index];
			}

			template <typename ... T_args>
			void add(T_args && ... p_args) {
				m_items.emplace_back(just::separator{}, std::forward<T_args>(p_args) ...);
			}

			void set(const var::cell & v_ptr) {
				m_cell = v_ptr;
			}

			void push() {
				m_items.emplace_back();
				*m_items.back() = *m_cell;
				m_cell = m_empty_cell;
			}

			void push_link() {
				m_items.emplace_back(m_cell);
				m_cell = m_empty_cell;
			}

			void pop() {
				m_items.pop_back();
			}

			void assign() {
				*m_items.back() = *m_cell;
			}

			void link() {
				m_items.back() = m_cell;
			}
		};

		//

		struct pos_module_aspect {
			// data
			t_index
				m_module_id = 0,
				m_aspect_id = 0;
		};

		struct pos_action {
			// data
			t_index
				m_group_id = 0,
				m_action_id = 0;
		};

		struct group_space : public var::optr_nest::is_target<group_space> {
			using pointer = group_space *;
			using t_vars = var::optr_nest::o_vector<var::value>;

			// data
			pos_action
				m_act_pos;
			t_vars
				m_vars;

			group_space(pos_action p_act_pos, sequence_pointer p_seq);
		};

		struct seq_space {
			using pointer = seq_space *;
			using t_items = std::vector<group_space>;

			// data
			pos_module_aspect
				m_seq_pos;
			t_items
				m_group_spaces;

			seq_space(pos_module_aspect p_seq_pos, space_pointer p_space);

			t_items::reference back() { return m_group_spaces.back(); }
		};

		struct run_space {
			using pointer = run_space *;
			using t_items = std::vector<seq_space>;

			// data
			space_pointer
				m_space;
			log_base::pointer
				m_log;
			t_items
				m_call_stack;

			run_space(space_pointer p_space, log_base::pointer p_log) : m_space{p_space}, m_log{p_log} {
				m_call_stack.emplace_back(pos_module_aspect{0, 0}, p_space);
			}

			t_items::reference back() { return m_call_stack.back(); }

			pos_action & act_pos(t_index p_seq_space_id, t_index p_group_space_id) {
				return m_call_stack[p_seq_space_id].m_group_spaces[p_group_space_id].m_act_pos;
			}

			void run();
		};

		//

		struct action_data {
			using t_cref = const action_data &;

			// data
			t_pos
				m_pos;
			t_integer
				m_value = 0;
			pos_module_aspect
				m_aspect_pos;
		};

		struct action_type {
			using const_pointer = const action_type *;

			static void do_nothing(run_space::pointer p_call, stack & p_stack, action_data::t_cref p_data) {}

			using t_fn = decltype(&do_nothing);

			// data
			t_text
				m_name;
			t_fn
				m_fn = &do_nothing;
		};

		struct action {
			using pointer = action *;
			using t_cref = const action &;

			// data
			action_type::const_pointer
				m_type;
			action_data
				m_data;

			bool empty() { return m_type->m_fn == &action_type::do_nothing; }
		};

		struct action_group {
			using pointer = action_group *;
			using t_items = std::vector<action>;
			using t_var_names = std::vector<t_text>;
			using t_var_names_ptr = std::unique_ptr<t_var_names>;

			// data
			t_items
				m_actions;
			t_var_names_ptr
				m_var_names;

			t_index var_count() const {
				return static_cast<bool>(m_var_names) ? std::ssize(*m_var_names) : 0;
			}

			t_index var_add(t_text p_name) {
				if( ! static_cast<bool>(m_var_names) ) {
					m_var_names = std::make_unique<t_var_names>();
				}
				t_index ret = std::ssize(*m_var_names);
				m_var_names->emplace_back(p_name);
				return ret;
			}
		};

		struct sequence {
			using pointer = sequence *;
			using t_items = std::vector<action_group>;

			// data
			fs::path
				m_path;
			t_items
				m_groups;

			sequence(fs::path p_path) : m_path{p_path} {
				m_groups.emplace_back();
				//m_groups.front().var_add(L"ret"s);
			}

			void clear() {
				m_groups.clear();
				m_groups.emplace_back();
			}
		};

		//

		struct actions {
			static void do_mod_var(run_space::pointer p_call, stack & p_stack, action_data::t_cref p_data);

			static void do_mod_var_link(run_space::pointer p_call, stack & p_stack, action_data::t_cref p_data);

			static void do_mod_var_ready(run_space::pointer p_call, stack & p_stack, action_data::t_cref p_data);

			static void do_put_int(run_space::pointer p_call, stack & p_stack, action_data::t_cref p_data) {
				p_stack.add(p_data.m_value);
			}

			static void do_assign(run_space::pointer p_call, stack & p_stack, action_data::t_cref p_data) {
				*p_stack.last() = *p_stack.pre_last();
				p_stack.pop();
			}

			static inline const action_type
				s_nothing		{L"do_nothing"s,		&action_type::do_nothing},
				s_mod_var_link	{L"do_mod_var_link"s,	&do_mod_var_link},
				s_put_int		{L"do_put_int"s,		&do_put_int},
				s_assign		{L"do_assign"s,			&do_assign},
				s_mod_var_ready	{L"do_mod_var_ready"s,	&do_mod_var_ready}
			;
		}; // actions

		//

		just::output_base & operator << (just::output_base & p_out, const pos_module_aspect & p_pos) {
			p_out << '{' << p_pos.m_module_id << ',' << p_pos.m_aspect_id << '}';
			return p_out;
		}

		just::output_base & operator << (just::output_base & p_out, const action & p_action) {
			p_out
				<< p_action.m_type->m_name
				<< ' ' << p_action.m_data.m_pos
				<< ' ' << p_action.m_data.m_value
				<< ' ' << p_action.m_data.m_aspect_pos
			;
			return p_out;
		}

		just::output_base & operator << (just::output_base & p_out, const action_group & p_group) {
			for( const action & v_it : p_group.m_actions ) {
				p_out << v_it << just::g_new_line;
			}
			return p_out;
		}

		just::output_base & operator << (just::output_base & p_out, const sequence & p_seq) {
			for( t_index v_pos = 0; const action_group & v_it : p_seq.m_groups ) {
				p_out << "\tgroup " << v_pos << just::g_new_line;
				p_out << v_it << just::g_new_line;
				++v_pos;
			}
			return p_out;
		}

	} // ns

} // ns