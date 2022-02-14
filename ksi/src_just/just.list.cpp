module;

export module just.list;
export import just.aux;
import <type_traits>;
import <utility>;

export namespace just {

template <typename Node>
concept c_list_node = requires(Node * nd) {
	nd = nd->next_;
	nd = nd->prev_;
};

template <typename T>
concept c_list_node_close_case = c_any_of<T, case_default, case_cross, case_none>;

namespace bases {

template <typename Target>
struct list_node {
	Target * next_, prev_;
};

} // ns bases

namespace detail {

// itarator

template <c_list_node Node, bool Is_reverse = false>
struct list_iterator {
	using type = Node;
	using pointer = type *;

	// data
	pointer current_;

	list_iterator(pointer node) : current_(node) {}

	void operator ++ () {
		if constexpr( Is_reverse )
		{ current_ = current_->prev_; }
		else
		{ current_ = current_->next_; }
	}
	pointer operator * () const { return current_; }
	bool operator != (pointer it) const { return current_ != it; }
};

template <c_list_node Node>
struct list_helper {
	using t_node = Node;
	using pointer = t_node *;

	static void connect(pointer node, pointer next) {
		node->next_ = next;
		next->prev_ = node;
	}
};

template <typename List>
auto list_append(List * to) {
	return [to](auto node) { to->append(node); return list_append(to); };
}
template <typename List>
auto list_insert_after(List * to, typename List::pointer pos) {
	return [to, pos](auto node) { to->insert_after(pos, node); return list_insert_after(to, node); };
}

} // ns detail

// list

template <c_list_node Node, template <typename T1> typename Closer = closers::simple_one>
struct list {
	using t_node = Node;
	using pointer = t_node *;
	using t_closer = Closer<t_node>;
	using t_helper = detail::list_helper<t_node>;
	//
	using t_iterator = detail::list_iterator<t_node>;
	using t_reverse_iterator = detail::list_iterator<t_node, true>;
	//
	using t_sentinel = pointer;
	using t_range = range<t_iterator, t_sentinel>;
	using t_reverse_range = range<t_reverse_iterator, t_sentinel>;

	static constexpr bool is_node_custom = std::is_base_of_v<bases::list_node<t_node>, t_node>;
	static constexpr bool is_special = ! std::is_same_v< t_closer, closers::simple_none<t_node> >;

	// data
	pointer head_ = nullptr, tail_ = nullptr;

	// no copy
	list(const list &) = delete;
	list & operator = (const list &) = delete;

	//
	list() = default;
	~list() requires(is_special) { clear(); }
	~list() requires(!is_special) = default;

	// iteration
	t_iterator begin() const { return head_; }
	t_sentinel end() const { return nullptr; }
	t_range get_range() const { return {head_, nullptr}; }
	t_reverse_range get_reverse_range() const { return {tail_, nullptr}; }
	static t_range get_range(pointer from) { return {from, nullptr}; }
	static t_range get_range(pointer from, pointer to) { return {from, to->next_}; }
	static t_reverse_range get_reverse_range(pointer from) { return {from, nullptr}; }
	static t_reverse_range get_reverse_range(pointer from, pointer to) { return {from, to->prev_}; }

	operator bool () const { return head_; }
	bool operator ! () const { return !head_; }

	list & clear() {
		if constexpr( is_special )
		while( tail_ ) {
			pointer prev = tail_->prev_;
			t_closer::close(tail_);
			tail_ = prev;
		} else tail_ = nullptr;
		head_ = nullptr;
		return *this;
	}
	auto append(pointer node) {
		node->next_ = nullptr;
		if( tail_ ) t_helper::connect(tail_, node);
		else { head_ = node; node->prev_ = nullptr; }
		tail_ = node;
		return detail::list_append(this);
	}
	auto prepend(pointer node) {
		node->prev_ = nullptr;
		if( head_ ) t_helper::connect(node, head_);
		else { tail_ = node; node->next_ = nullptr; }
		head_ = node;
		return detail::list_insert_after(this, node);
	}
	auto insert_after(pointer pos, pointer node) {
		if( pos ) {
			if( pointer next = pos->next_ ) t_helper::connect(node, next);
			else { tail_ = node; node->next_ = nullptr; }
			t_helper::connect(pos, node);
		} else prepend(node);
		return detail::list_insert_after(this, node);
	}
	auto insert_before(pointer pos, pointer node) {
		if( pos ) {
			if( pointer prev = pos->prev_ ) t_helper::connect(prev, node);
			else { head_ = node; node->prev_ = nullptr; }
			t_helper::connect(node, pos);
		} else append(node);
		return detail::list_insert_after(this, node);
	}
	list & remove_first() {
		if( pointer it = head_ ) {
			head_ = it->next_;
			if( head_ ) head_->prev_ = nullptr;
			else tail_ = nullptr;
			if constexpr( is_special ) t_closer::close(it);
		}
		return *this;
	}
	list & remove_last() {
		if( pointer it = tail_ ) {
			tail_ = it->prev_;
			if( tail_ ) tail_->next_ = nullptr;
			else head_ = nullptr;
			if constexpr( is_special ) t_closer::close(it);
		}
		return *this;
	}
	list & remove_after(pointer pos) {
		if( pointer it = pos->next_ ) {
			if( pointer next = it->next_ ) t_helper::connect(pos, next);
			else { tail_ = pos; pos->next_ = nullptr; }
			if constexpr( is_special ) t_closer::close(it);
		}
		return *this;
	}
	list & remove_before(pointer pos) {
		if( pointer it = pos->prev_ ) {
			if( pointer prev = it->prev_ ) t_helper::connect(prev, pos);
			else { head_ = pos; pos->prev_ = nullptr; }
			if constexpr( is_special ) t_closer::close(it);
		}
		return *this;
	}
	list & remove(pointer pos) {
		pointer next = pos->next_;
		if( pointer prev = pos->prev_ ) {
			if( next ) t_helper::connect(prev, next);
			else { tail_ = prev; prev->next_ = nullptr; }
		} else {
			head_ = next;
			if( next ) next->prev_ = nullptr;
			else tail_ = nullptr;
		}
		if constexpr( is_special ) t_closer::close(pos);
		return *this;
	}
};

// simple/cross/none detail

namespace detail {

template <typename T, typename Target = case_default>
struct list_node {
	using pointer = std::conditional_t<std::is_same_v<Target, case_default>, list_node *, Target *>;
	
	// data
	T value_;
	pointer next_, prev_;
};

template <typename T>
struct list_node_cross :
	public list_node<T, list_node_cross<T> >,
	public bases::with_deleter< list_node_cross<T> >
{};

template <typename T, c_list_node_close_case Node_close_case,
	template <c_list_node Node1, template <typename T1> typename Closer1> typename List
>
using list_alias = std::conditional_t<std::is_same_v<Node_close_case, case_default>,
	List<list_node<T>, closers::simple_one>, std::conditional_t<std::is_same_v<Node_close_case, case_none>,
	List<list_node<T>, closers::simple_none>,
	List<list_node_cross<T>, closers::simple_call_deleter>
> >;

template <c_list_node Node, template <typename T1> typename Closer>
struct list_cross :
	public list<Node, Closer>,
	public bases::with_deleter< list_cross<Node, Closer> >
{
	using t_list = list<Node, Closer>;
	using t_list::t_list;
};

} // ns detail

// simple/cross/none

template <typename T, bool List_cross = false, c_list_node_close_case Node_close_case = case_default>
using list_alias = std::conditional_t<List_cross,
	detail::list_alias<T, Node_close_case, detail::list_cross>,
	detail::list_alias<T, Node_close_case, list>
>;

// actions

template <typename List>
auto list_append(List & to) {
	return [h = &to]<typename ... Params>(Params && ... args) {
		if constexpr( List::is_node_custom ) {
			h->append( new List::t_node{std::forward<Params>(args) ...} );
		} else {
			h->append( new List::t_node{ {std::forward<Params>(args) ...} } );
		}
		return list_append(*h);
	};
}
template <typename List>
auto list_insert_after(List & to, typename List::pointer pos) {
	return [h = &to, pos]<typename ... Params>(Params && ... args) {
		typename List::pointer node;
		if constexpr( List::is_node_custom ) {
			h->insert_after( pos, node = new List::t_node{std::forward<Params>(args) ...} );
		} else {
			h->insert_after( pos, node = new List::t_node{ {std::forward<Params>(args) ...} } );
		}
		return list_insert_after(*h, node);
	};
}
template <typename List>
auto list_insert_before(List & to, typename List::pointer pos) {
	return [h = &to, pos]<typename ... Params>(Params && ... args) {
		typename List::pointer node;
		if constexpr( List::is_node_custom ) {
			h->insert_before( pos, node = new List::t_node{std::forward<Params>(args) ...} );
		} else {
			h->insert_before( pos, node = new List::t_node{ {std::forward<Params>(args) ...} } );
		}
		return list_insert_after(*h, node);
	};
}
template <typename List>
auto list_prepend(List & to) {
	return [h = &to]<typename ... Params>(Params && ... args) {
		typename List::pointer node;
		if constexpr( List::is_node_custom ) {
			h->prepend( node = new List::t_node{std::forward<Params>(args) ...} );
		} else {
			h->prepend( node = new List::t_node{ {std::forward<Params>(args) ...} } );
		}
		return list_insert_after(*h, node);
	};
}

} // ns just