module ksi.act;

#include "../src/pre.h"

import ksi.space;

namespace ksi {

	namespace act {
	
		void run_space::run() {
			stack p_stack;
			while( m_call_stack.size() > 0 ) {
				seq_space::pointer v_seq_space = &m_call_stack.back();
				t_module::pointer v_mod = m_space->mod_get(v_seq_space->m_seq_pos.m_module_id);
				sequence::pointer v_seq = &v_mod->m_seqs[v_seq_space->m_seq_pos.m_seq_id];
				if( v_seq_space->m_act_poses.size() > 0 ) {
					pos_action::pointer v_pos_action = &v_seq_space->back();
					action_group::pointer v_group = &v_seq->m_groups[v_pos_action->m_group_id];
					if( v_pos_action->m_action_id < std::ssize(v_group->m_actions) ) {
						action::pointer v_action = &v_group->m_actions[v_pos_action->m_action_id];
						v_action->m_type->m_fn(this, p_stack, v_action->m_data);
						++v_pos_action->m_action_id;
					} else {
						v_seq_space->m_act_poses.pop_back();
					}
				} else {
					m_call_stack.pop_back();
				}
			}
		}
	
	} // ns

} // ns