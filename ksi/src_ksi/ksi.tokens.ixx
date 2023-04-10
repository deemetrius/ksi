module;

#include "../src/pre.h"

export module ksi.tokens;

export import ksi.ast;
export import <list>;

export namespace ksi {

	namespace tokens {

		enum class mode { n_default, n_target };

		struct token_info {
			// data
			mode
				m_mode = mode::n_default;
		};

		struct token_base : public just::with_deleter<token_base> {
			// data
			token_info
				m_info;

			token_base(token_info p_info) : m_info{p_info} {}

			virtual ~token_base() {}

			virtual void perform(ast::prepare_data & p_data) = 0;
		};

		template <typename T_data>
		struct token_exact : public token_base {
			// data
			T_data
				m_data;

			token_exact(T_data && p_data, token_info p_info = {}) :
				token_base{p_info},
				m_data{std::move(p_data)}
			{}

			void perform(ast::prepare_data & p_data) override {
				m_data.rule_perform(p_data, m_info);
			}
		};

		//

		struct nest {
			using t_ptr = std::unique_ptr<token_base, just::hold_deleter>;
			using t_items = std::list<t_ptr>;

			// data
			t_items
				m_items;

			template <typename T_data>
			void add(T_data * p_data) {
				using t_item = token_exact<T_data>;
				m_items.emplace_back( std::make_unique<t_item>( std::move(*p_data) ) );
			}

			void go(ast::prepare_data & p_data) {
				for( t_ptr & v_it : m_items ) {
					v_it->perform(p_data);
				}
			}
		};

	} // ns

} // ns