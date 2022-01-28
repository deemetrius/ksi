module;

export module just.dict;
export import just.array;
import <compare>;
import <type_traits>;
import <numeric>;

export namespace just {

template <typename Key, typename Value>
struct dict_node {
	using t_key = const Key;
	using t_value = Value;
	using t_ordering = std::compare_three_way_result_t<t_key>;

	// data
	t_key key_;
	t_value value_;

	// pair to pair
	friend t_ordering operator <=> (const dict_node & v1, const dict_node & v2) { return v1.key_ <=> v2.key_; }
	friend bool operator == (const dict_node & v1, const dict_node & v2) { return v1.key_ == v2.key_; }
	// pair to key
	friend t_ordering operator <=> (const dict_node & v1, t_key & v2) { return v1.key_ <=> v2; }
	friend bool operator == (const dict_node & v1, t_key & v2) { return v1.key_ == v2; }
	// key to pair
	friend t_ordering operator <=> (t_key & v1, const dict_node & v2) { return v1 <=> v2.key_; }
	friend bool operator == (t_key & v1, const dict_node & v2) { return v1 == v2.key_; }
};

template <typename T>
using arg_passing_t = std::conditional_t<std::is_scalar_v<T>, T, const T &>;

template <typename Data>
struct dict_find_result {
	id index_;
	bool flag_ = false;
	Data * data_ = nullptr;

	operator bool () const { return flag_; }
	bool operator ! () const { return !flag_; }
	Data * operator -> () const { return data_; }
};

template <typename Key, typename Value, typename Capacity>
struct dict {
	using t_capacity = Capacity;
	using t_node = dict_node<Key, Value>;
	using t_key = t_node::t_key;
	using t_value = t_node::t_value;
	using t_pass_key = arg_passing_t<t_key>;
	using t_pass_value = arg_passing_t<t_value>;
	using t_ordering = t_node::t_ordering;
	using t_internal = array<t_node, t_capacity, closers::simple_destructor>;
	using pointer = t_internal::pointer;
	using t_find_result = dict_find_result<t_node>;

	static t_find_result find(const t_internal & to, t_pass_key key) {
		// empty
		if( !to ) return {0};
		// last
		id right = to->count_ -1;
		pointer current = to.data() + right;
		t_ordering order = key <=> *current;
		if( order == t_ordering::greater ) return {to->count_};
		if( order == t_ordering::equivalent ) return {right, true, current};
		// count 1
		if( !right ) return {0};
		// first
		current = to.data();
		order = key <=> *current;
		if( order == t_ordering::less ) return {0};
		if( order == t_ordering::equivalent ) return {0, true, current};
		// count 2
		if( right == 1 ) return {1};
		--right;
		id left = 1, mid;
		do {
			mid = std::midpoint(left, right);
			current = to.data() + mid;
			order = key <=> *current;
			if( order == t_ordering::greater ) left = mid + 1;
			else if( order == t_ordering::less ) right = mid - 1;
			else return {mid, true, current};
		} while( left <= right );
		return {mid + (order > 0 ? 1 : 0)};
	}

	static t_find_result add(t_internal & to, t_pass_key key, t_pass_value value) {
		t_find_result res = find(to, key);
		if( res ) {
			res->value_ = value;
		} else {
			just::array_insert_guard ag(to, res.index_);
			res.data_ = ag->place;
			res.flag_ = true;
			new( ag->place ) t_node{key, value};
		}
		return res;
	}

	static bool remove(t_internal & to, t_pass_key key) {
		t_find_result res = find(to, key);
		if( res ) array_remove_n(to, res.index_);
		return res;
	}
};

} // ns just