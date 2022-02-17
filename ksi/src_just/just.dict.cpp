module;

export module just.dict;
export import just.array;
import just.compare;
import <compare>;
import <type_traits>;
import <numeric>;

export namespace just {

template <typename Pointer>
struct dict_find_result {
	id index_;
	Pointer element_ = nullptr;
	bool was_added_ = false;

	operator bool () const { return element_; }
	bool operator ! () const { return !element_; }

	Pointer operator -> () const { return element_; }
};

template <typename Key, typename Value>
struct dict_pair {
	const Key key_;
	Value value_;
};

namespace detail {

template <typename Container, typename Key,
	template <typename Pointer1, typename Key1> typename Key_helper
>
struct dict_base {
	using t_key = Key;
	using t_pass_key = arg_passing_t<t_key>;
	using t_ordering = std::compare_three_way_result_t<t_key>;
	using t_helper = compare_helper<t_ordering>;
	//
	using t_internal = Container;
	using pointer = t_internal::pointer;
	using t_find_result = dict_find_result<pointer>;
	//
	using traits_key = Key_helper<pointer, t_key>;

	static t_find_result find(const t_internal & to, t_pass_key key) {
		// empty
		if( !to ) return {0};
		// last
		id right = to->count_ -1;
		pointer begin = to.data(), current = begin + right;
		t_ordering order = key <=> traits_key::get_key(current);
		if( order == t_helper::greater ) return {to->count_};
		if( order == t_helper::equal ) return {right, current};
		// count 1
		if( !right ) return {0};
		// first
		current = begin;
		order = key <=> traits_key::get_key(current);
		if( order == t_helper::less ) return {0};
		if( order == t_helper::equal ) return {0, current};
		// count 2
		if( right == 1 ) return {1};
		--right;
		id left = 1, mid;
		do {
			mid = std::midpoint(left, right);
			current = begin + mid;
			order = key <=> traits_key::get_key(current);
			if( order == t_helper::greater ) left = mid + 1;
			else if( order == t_helper::less ) right = mid - 1;
			else return {mid, current};
		} while( left <= right );
		return {mid + (order > 0 ? 1 : 0)};
	}

	static bool remove(t_internal & to, t_pass_key key) {
		if( t_find_result res = find(to, key) ) {
			array_remove_n(to, res.index_);
			return true;
		}
		return false;
	}

	static bool contains(const t_internal & to, t_pass_key key) { return find(to, key); }
};

//

template <typename Pointer, typename Key>
struct dict_pair_key_helper {
	using t_pass_key = arg_passing_t<Key>;

	static inline t_pass_key get_key(Pointer h) { return h->key_; }
};

template <typename Key, typename Value, typename Capacity>
struct dict_pair_helper {
	using t_element = dict_pair<Key, Value>;
	using t_internal = array_alias<t_element, Capacity>;
	using use_base = dict_base<t_internal, Key, dict_pair_key_helper>;
};

//

template <typename Pointer, typename Key>
struct dict_set_key_helper {
	using t_pass_key = arg_passing_t<Key>;

	static inline t_pass_key get_key(Pointer h) { return *h; }
};

template <typename Key, typename Capacity>
struct dict_set_helper {
	using t_element = Key;
	using t_internal = array_alias<t_element, Capacity>;
	using use_base = dict_base<t_internal, Key, dict_set_key_helper>;
};

} // ns detail

template <typename Key, typename Value, typename Capacity>
struct dict :
	public detail::dict_pair_helper<Key, Value, Capacity>::use_base
{
	using helper = detail::dict_pair_helper<Key, Value, Capacity>;
	using t_element = helper::t_element;
	using t_internal =  helper::t_internal;
	using base = helper::use_base;
	using typename base::pointer;
	using typename base::t_find_result;
	//
	using t_key = Key;
	using t_value = Value;
	using t_pass_key = arg_passing_t<t_key>;
	using t_pass_value = arg_passing_t<t_value>;

	template <c_capacity_more Case_capacity_more = case_default>
	static t_find_result add(t_internal & to, t_pass_key key, t_pass_value value) {
		t_find_result res = base::find(to, key);
		if( res ) {
			res->value_ = value;
		} else {
			{
				just::array_insert_guard<t_internal, Case_capacity_more> ag(to, res.index_);
				new( res.element_ = ag->place ) t_element{key, value};
			}
			res.was_added_ = true;
		}
		return res;
	}
};

template <typename Key, typename Capacity>
struct dict_set :
	public detail::dict_set_helper<Key, Capacity>::use_base
{
	using helper = detail::dict_set_helper<Key, Capacity>;
	using t_element = helper::t_element;
	using t_internal =  helper::t_internal;
	using base = helper::use_base;
	using typename base::pointer;
	using typename base::t_find_result;
	//
	using t_key = Key;
	using t_pass_key = arg_passing_t<t_key>;

	template <c_capacity_more Case_capacity_more = case_default>
	static t_find_result add(t_internal & to, t_pass_key key) {
		t_find_result res = base::find(to, key);
		if( !res ) {
			{
				just::array_insert_guard<t_internal, Case_capacity_more> ag(to, res.index_);
				new( res.element_ = ag->place ) t_element(key);
			}
			res.was_added_ = true;
		}
		return res;
	}
};

} // ns just