module;

#include "../src/pre.h"

export module ksi.log;

export import <filesystem>;
import <utility>;
export import just.text;
export import just.list;
export import just.output;
//export import just.files;

export namespace ksi {

	namespace fs = std::filesystem;

	using t_text_value = just::text;
	using t_text_value_pointer = t_text_value *;

	struct position {
		// data
		just::t_index	m_line = 0;
		just::t_index	m_char = 0;
	};
	
	struct log_message {
		// data
		fs::path		m_path;
		t_text_value	m_message;
		position		m_pos;
	};

	struct log_pos {
		// data
		fs::path	m_path;
		position	m_pos;

		log_message message(const t_text_value & p_message) const { return {m_path, p_message, m_pos}; }
	};

	struct log_base {
		using pointer = log_base *;
		
		virtual ~log_base() = default;
		
		virtual void add(log_message && p_message) {}
		virtual void out(just::output_base & p_out) {}
	};

	using log_pointer = log_base *;

	struct log_list : public log_base {
		struct t_node : public just::node_list<t_node> {
			// data
			log_message		m_message;
			
			t_node(log_message && p_message) : m_message{std::move(p_message)} {}
		};

		using t_list = just::list<t_node>;

		// data
		t_list	m_list;

		void add(log_message && p_message) override {
			m_list.append(new t_node{std::move(p_message)});
		}

		void out(just::output_base & p_out) override {
			m_list.m_zero.node_apply_to_others(
				[&p_out](t_node::node_pointer p_node){
					typename t_node::target_pointer v_node = p_node->node_get_target();
					p_out,
					'[', v_node->m_message.m_pos.m_line,
					':', v_node->m_message.m_pos.m_char, "] ",
					v_node->m_message.m_path.c_str(), just::g_new_line,
					v_node->m_message.m_message->m_text, just::g_new_line;
				}
			);
		}
	};

} // ns