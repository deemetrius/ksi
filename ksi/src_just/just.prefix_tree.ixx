module;

#include "../src/pre.h"

export module just.prefix_tree;

import <type_traits>;
export import <map>;
export import <optional>;
export import <memory>;
export import <string_view>;

export namespace just {

	template <typename T_key, typename T_value>
	struct prefix_tree {
		using t_key = T_key;
		using t_value = T_value;
		using t_value_pointer = t_value *;

		static constexpr bool s_key_scalar = std::is_scalar_v<t_key>;
		static constexpr bool s_key_char = std::is_same_v<t_key, char> || std::is_same_v<t_key, wchar_t>;

		using t_pass_key = std::conditional<s_key_scalar, t_key, const t_key &>;
		using t_key_view = std::conditional<s_key_char, std::basic_string_view<t_key>, bool>;

		struct t_pair {
			// data
			t_key_view	m_key;
			t_value		m_value;
		};

		struct t_node {
			using pointer = t_node *;
			using t_ptr = std::unique_ptr<t_node>;
			using t_map = std::map<t_key, t_ptr>;
			using t_value = std::optional<t_value>;

			// data
			t_map		m_map;
			t_value		m_value;

			pointer find(t_pass_key p_key) {
				typename t_map::iterator v_it = m_map.find(p_key);
				return (v_it == m_map.end() ) ? nullptr : (*v_it).get();
			}

			pointer sure(t_pass_key p_key) {
				pointer ret = find(p_key);
				if( ret ) { return ret; }
				t_ptr v_ptr = std::make_unique<t_node>();
				ret = v_ptr.get();
				m_map.try_emplace(p_key, std::move(v_ptr) );
				return ret;
			}

			t_value_pointer done() { return m_value.has_value() ? &m_value.value() : nullptr; }
		};

		// data
		t_node	m_root;

		prefix_tree() = default;
		prefix_tree(std::initializer_list<t_pair> p_init) requires( s_key_char ) {
			for( t_pair & v_it : p_init ) {
				typename t_node::pointer v_node = &m_root;
				for( t_key & v_key : v_it.m_key ) {
					v_node = v_node->sure(v_key);
				}
				v_node->m_value = std::move(v_it.m_value);
			}
		}

		t_node::pointer find(t_pass_key p_key) { return m_root.find(p_key); }
	};

} // ns