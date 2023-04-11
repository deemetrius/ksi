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
			m_act_poses.emplace_back(pos_action{}, p_space->seq_get(p_seq_pos) );
		}

		void run_space::run() {
			stack p_stack;
			while( m_call_stack.size() > 0 ) {
				seq_space::pointer v_seq_space = &m_call_stack.back();
				t_module::pointer v_mod = m_space->mod_get(v_seq_space->m_seq_pos.m_module_id);
				sequence::pointer v_seq = &v_mod->m_seqs[v_seq_space->m_seq_pos.m_aspect_id];
				if( v_seq_space->m_act_poses.size() > 0 ) {
					group_space::pointer v_group_space = &v_seq_space->back();
					pos_action::pointer v_pos_action = &v_group_space->m_act_pos;
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

		//

		void actions::do_mod_var(run_space::pointer p_call, stack & p_stack, action_data::t_cref p_data) {
			//p_stack.set( p_call->m_space->var_get(p_data.m_aspect_pos) );
			//p_stack.push();
			// todo: lazy init
		}

		void actions::do_mod_var_link(run_space::pointer p_call, stack & p_stack, action_data::t_cref p_data) {
			p_stack.set( p_call->m_space->var_get(p_data.m_aspect_pos) );
			p_stack.push_link();
		}

	} // ns

} // ns