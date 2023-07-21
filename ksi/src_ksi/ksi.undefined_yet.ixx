module;

#include "../src/pre.h"

export module ksi.undefined_yet;

export import ksi.space;
export import <compare>;

export namespace ksi { namespace ast {

	struct undefined_yet {
		using action_pointer = act::action::pointer;

		struct unknown_property {
			using t_items = just::vector<action_pointer, just::grow_step<8, 7> >;

			// data
			t_items
				m_items;

			void add(action_pointer p_action) { m_items.emplace_back(p_action); }

			void set(act::pos_module_aspect p_aspect_pos) {
				for( action_pointer v_it : m_items ) {
					v_it->m_data.m_aspect_pos = p_aspect_pos;
				}
			}
		};

		/*struct map_key {
			// data
			text_str
				module_name,
				var_name;

			friend bool operator == (const map_key &, const map_key &) = default;
			friend std::strong_ordering operator <=> (const map_key &, const map_key &) = default;
		};*/

		using t_items = std::map</*map_key*/ text_str, unknown_property, std::ranges::less>;
		using t_emplace_result = std::pair<t_items::iterator, bool>;

		// data
		t_items
			m_items;

		void unkown(/*t_text p_module_name,*/ t_text p_var_name, action_pointer p_action) {
			t_emplace_result v_res = m_items.try_emplace(/*{*p_module_name, *p_var_name}*/ *p_var_name);
			v_res.first->second.add(p_action);
		}

		bool known(/*t_text p_module_name,*/ t_text p_var_name, act::pos_module_aspect p_aspect_pos) {
			t_items::iterator v_it = m_items.find(/*{*p_module_name, *p_var_name}*/ *p_var_name);
			if( v_it != m_items.end() ) {
				v_it->second.set(p_aspect_pos);
				m_items.erase(v_it);
				return true;
			}
			return false;
		}
	};

} } // ns ns