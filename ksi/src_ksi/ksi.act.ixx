module;

#include "../src/pre.h"

export module ksi.act;

export import ksi.var;
export import ksi.log;
export import <vector>;

export namespace ksi {

	struct space;

	namespace act {

		using namespace std::string_literals;

		using space_pointer = space *;

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

		struct seq_space {
			using pointer = seq_space *;
			using t_items = std::vector<pos_action>;

			// data
			pos_seq
				m_seq_pos;
			t_items
				m_act_poses;

			seq_space(pos_seq p_seq_pos) : m_seq_pos{p_seq_pos} {
				m_act_poses.emplace_back();
			}

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
				m_call_stack.emplace_back(pos_seq{0, 0});
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

			// data
			t_items
				m_actions;
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
			}
		};

	} // ns

} // ns