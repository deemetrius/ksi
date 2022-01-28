module;

export module just.dict;
export import just.array;
import <compare>;
import <type_traits>;
import <numeric>;

export namespace just {

template <typename Key, typename Value>
struct dict_node {
	const Key key_;
	Value value_;
};

template <typename T, bool Can_change = false>
using arg_passing_t = std::conditional_t<std::is_scalar_v<T>, T,
	std::conditional_t<Can_change, T &, const T &>
>;

template <typename Data>
struct dict_find_result {
	id index_;
	bool flag_ = false;
	Data * data_ = nullptr;

	operator bool () const { return flag_; }
	bool operator ! () const { return !flag_; }

	Data * operator -> () const { return data_; }
};

struct traits_dict_store_node {
	template <typename Key, typename Value, typename Capacity>
	struct inner {
		using t_node = dict_node<Key, Value>;
		using t_key = Key;
		using t_value = Value;
		using t_pass_key = arg_passing_t<t_key>;
		using t_pass_value = arg_passing_t<t_value>;
		using t_internal = array<t_node, Capacity, closers::simple_destructor>;
		using pointer = t_internal::pointer;
		using t_find_result = dict_find_result<t_node>;

		static inline t_pass_key get_key(pointer h) {
			return h->key_;
		}

		static inline void assign_new(pointer place, t_pass_key key, t_pass_value value) {
			new( place ) t_node{key, value};
		}

		static inline void change_existing(t_find_result & res, t_pass_key key, t_pass_value value) {
			res->value_ = value;
		}
	};
};

template <typename Key, typename Value, typename Capacity, typename Traits = traits_dict_store_node>
struct dict {
	using traits = Traits::template inner<Key, Value, Capacity>;
	using t_key = traits::t_key;
	using t_value = traits::t_value;
	using t_pass_key = traits::t_pass_key;
	using t_pass_value = traits::t_pass_value;
	using t_internal = traits::t_internal;
	using pointer = traits::pointer;
	using t_find_result = traits::t_find_result;
	using t_ordering = std::compare_three_way_result_t<t_key>;

	static t_find_result find(const t_internal & to, t_pass_key key) {
		// empty
		if( !to ) return {0};
		// last
		id right = to->count_ -1;
		pointer current = to.data() + right;
		t_ordering order = key <=> traits::get_key(current);
		if( order == t_ordering::greater ) return {to->count_};
		if( order == t_ordering::equivalent ) return {right, true, current};
		// count 1
		if( !right ) return {0};
		// first
		current = to.data();
		order = key <=> traits::get_key(current);
		if( order == t_ordering::less ) return {0};
		if( order == t_ordering::equivalent ) return {0, true, current};
		// count 2
		if( right == 1 ) return {1};
		--right;
		id left = 1, mid;
		do {
			mid = std::midpoint(left, right);
			current = to.data() + mid;
			order = key <=> traits::get_key(current);
			if( order == t_ordering::greater ) left = mid + 1;
			else if( order == t_ordering::less ) right = mid - 1;
			else return {mid, true, current};
		} while( left <= right );
		return {mid + (order > 0 ? 1 : 0)};
	}

	static t_find_result add(t_internal & to, t_pass_key key, t_pass_value value) {
		t_find_result res = find(to, key);
		if( res ) {
			traits::change_existing(res, key, value);
		} else {
			just::array_insert_guard ag(to, res.index_);
			traits::assign_new(ag->place, key, value);
			res.data_ = ag->place;
			res.flag_ = true;
		}
		return res;
	}

	static bool remove(t_internal & to, t_pass_key key) {
		if( t_find_result res = find(to, key) ) {
			array_remove_n(to, res.index_);
			return true;
		}
		return false;
	}
};

} // ns just