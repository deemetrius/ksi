module;

#include "../src/pre.h"

export module ksi.log;

export import ksi.var;
export import <filesystem>;

export namespace ksi {

	namespace fs = std::filesystem;

	struct t_pos {
		// data
		t_integer
			m_line = 1,
			m_col = 1;
	};

	just::output_base & operator << (just::output_base & p_out, const t_pos & p_pos) {
		p_out << '[' << p_pos.m_line << ':' << p_pos.m_col << ']';
		return p_out;
	}

	struct log_pos {
		// data
		fs::path
			m_path;
		t_pos
			m_pos;

		log_pos & log_pos_only() { return *this; }
		const log_pos & log_pos_only() const { return *this; }
	};

	just::output_base & operator << (just::output_base & p_out, const log_pos & p_log_pos) {
		p_out << p_log_pos.m_pos << ' ' << p_log_pos.m_path.c_str();
		return p_out;
	}

	struct log_message : public log_pos {
		// data
		t_text
			m_msg;
	};

	struct log_base {
		using pointer = log_base *;

		virtual ~log_base() {}

		virtual void add(log_message && p_msg) = 0;
		virtual void out(just::output_base & p_out) = 0;
	};

	struct log_vector : public log_base {
		using t_items = std::vector<log_message>;

		// data
		t_items
			m_items;

		void add(log_message && p_msg) override {
			m_items.emplace_back( std::move(p_msg) );
		}

		void out(just::output_base & p_out = just::g_console) override {
			for( const log_message & v_it : m_items ) {
				p_out
					<< v_it.log_pos_only() << just::g_new_line
					<< v_it.m_msg->data() << just::g_new_line
				;
			}
		}
	};

} // ns