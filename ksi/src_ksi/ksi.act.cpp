module ksi.act;

#include "../src/pre.h"

import ksi.space;

namespace ksi {

	namespace act {

		group_space::group_space(pos_action p_act_pos, sequence::pointer p_seq) :
			m_act_pos{p_act_pos},
			m_vars{this->m_point.get(), p_seq->m_groups[p_act_pos.m_group_id].var_count()}
		{}

		seq_space::seq_space(pos_module_aspect p_seq_pos, space_pointer p_space) : m_seq_pos{p_seq_pos} {
			m_group_spaces.emplace_back(pos_action{}, p_space->seq_get(p_seq_pos) );
		}

		void run_space::run() {
			stack p_stack;
			//
			t_index v_call_stack_size = std::ssize(m_call_stack);
			while( v_call_stack_size > 0 ) {
				t_index v_seq_space_id = v_call_stack_size - 1;
				pos_module_aspect v_seq_pos = m_call_stack[v_seq_space_id].m_seq_pos;
				sequence::pointer v_seq = m_space->seq_get(v_seq_pos);
				//
				t_index v_act_poses_size = std::ssize(m_call_stack[v_seq_space_id].m_group_spaces);
				if( v_act_poses_size > 0 ) {
					t_index v_group_space_id = v_act_poses_size -1;
					pos_action v_action_pos = this->act_pos(v_seq_space_id, v_group_space_id);
					action_group::pointer v_group = &v_seq->m_groups[v_action_pos.m_group_id];
					if( v_action_pos.m_action_id < std::ssize(v_group->m_actions) ) {
						action::pointer v_action = v_group->m_actions[v_action_pos.m_action_id];
						++v_action_pos.m_action_id;
						this->act_pos(v_seq_space_id, v_group_space_id) = v_action_pos;
						v_action->m_type->m_fn(this, p_stack, v_action->m_data);
					} else {
						m_call_stack[v_seq_space_id].m_group_spaces.pop_back();
					}
				} else {
					m_call_stack.pop_back();
				}
			} // while
		}

		//

		void actions::do_mod_var(run_space::pointer p_call, stack & p_stack, action_data::t_cref p_data) {
			//p_stack.set( p_call->m_space->var_get(p_data.m_aspect_pos) );
			//p_stack.push();
			// todo: lazy init
		}

		void actions::do_mod_var_link(run_space::pointer p_call, stack & p_stack, action_data::t_cref p_data) {
			p_stack.set( p_call->m_space->var_get(p_data.m_aspect_pos).m_cell );
			p_stack.push_link();
		}

		void actions::do_mod_var_ready(run_space::pointer p_call, stack & p_stack, action_data::t_cref p_data) {
			p_call->m_space->var_get(p_data.m_aspect_pos).m_status = property_status::n_ready;
		}

	} // ns

} // ns