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

		struct stack {};

		//

		struct pos_seq {
			// data
			t_index
				m_module_id,
				m_seq_id;
		};

		struct pos_action {
			using pointer = pos_action *;

			// data
			t_index
				m_group_id = 0,
				m_action_id = 0;
		};

		struct group_space : public var::with_ring::is_owned<group_space> {
			using pointer = group_space *;
			using t_vars = var::with_ring::o_vector<var::value>;

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
			pos_seq
				m_seq_pos;
			t_items
				m_act_poses;

			seq_space(pos_seq p_seq_pos, space_pointer p_space);

			t_items::reference back() { return m_act_poses.back(); }
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
				m_call_stack.emplace_back(pos_seq{0, 0}, p_space);
			}

			t_items::reference back() { return m_call_stack.back(); }

			void run();
		};

		//

		struct action_data {
			using t_cref = const action_data &;

			// data
			t_pos
				m_pos;
			t_integer
				m_value = 0,
				m_id = 0;
			t_index
				m_module_id = 0;
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

		struct actions {
			static inline const action_type
				s_nothing{L"do_nothing"s, &action_type::do_nothing};
		};

		//

		struct action {
			using pointer = action *;

			// data
			action_type::const_pointer
				m_type = &actions::s_nothing;
			action_data
				m_data;
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

			sequence() {
				m_groups.emplace_back();
				m_groups.front().var_add(L"ret"s);
			}
		};

	} // ns

} // ns