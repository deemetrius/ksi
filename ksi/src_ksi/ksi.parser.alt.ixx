module;

#include "../src/pre.h"

export module ksi.parser:alt;

import :state;
import :basic;
export import <string>;
export import <vector>;

namespace ksi {

	using namespace std::string_literals;

	namespace parser {

		template <typename ... T_items>
		struct first_type;

		template <typename T_first, typename ... T_rest>
		struct first_type<T_first, T_rest ...> {
			using type = T_first;

			template <template <typename ...> typename T_template>
			using type_rest = T_template<T_rest ...>;
		};

		template <typename ... T_rules>
		struct rule_alt {
			using t_first = first_type<T_rules ...>::type;

			static constexpr bool s_take_rest = sizeof...(T_rules) > 1;

			using t_rest = std::conditional_t<s_take_rest,
				typename first_type<T_rules ...>::template type_rest<rule_alt>,
				void
			>;

			using t_items = std::vector<t_text>;

			static text_str message(state & p_state) {
				t_items v_items;
				add(p_state, v_items);
				return just::implode_items(v_items,
					text_str{L", "},
					text_str{L"Expected: "}
				);
			}

			using t_fn_message = decltype(&message);

			static void add(state & p_state, t_items & p_items) {
				if( t_first::check(p_state) ) { p_items.push_back(t_first::s_name); }
				if constexpr ( s_take_rest ) { t_rest::add(p_state, p_items); }
			}

			static bool inner_parse(state & p_state, tokens::nest & p_tokens, ast::prepare_data & p_data, t_fn_message p_fn) {
				if( t_first::check(p_state) ) {
					typename t_first::t_data v_data;
					if( v_data.parse(p_state, p_tokens, p_data) ) {
						v_data.action(p_state, p_tokens, p_data);
						p_state.m_was_space = std::is_same_v<t_first, t_opt_space>;
						return true;
					}
				}
				if constexpr ( s_take_rest ) {
					return t_rest::inner_parse(p_state, p_tokens, p_data, p_fn);
				} else {
					p_data.m_log->add( p_state.message( p_fn(p_state) ) );
				}
				return false;
			}

			static bool parse(state & p_state, tokens::nest & p_tokens, ast::prepare_data & p_data) {
				return inner_parse(p_state, p_tokens, p_data, &message);
			}
		};

	} // ns

} // ns