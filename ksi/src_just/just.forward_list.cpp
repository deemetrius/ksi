module;

export module just.forward_list;
export import just.aux;
import <type_traits>;
import <utility>;

export namespace just {

template <typename Node>
concept c_forward_list_node = requires(Node * nd) {
	nd = nd->next_;
};

namespace bases {

template <typename Target>
struct forward_list_node {
	Target * next_;
};

} // ns bases

namespace detail {

// itarator

template <c_forward_list_node Node>
struct forward_list_iterator {
	using type = Node;
	using pointer = type *;

	// data
	pointer current_;

	void operator ++ () { current_ = current_->next_; }
	pointer operator * () const { return current_; }
	bool operator != (pointer it) const { return current_ != it; }
};

template <typename List>
auto forward_list_append(List * to) {
	return [to](auto node) { to->append(node); return forward_list_append(to); };
}
template <typename List>
auto forward_list_insert_after(List * to, typename List::pointer pos) {
	return [to, pos](auto node) { to->insert_after(pos, node); return forward_list_insert_after(to, node); };
}
/*template <typename List>
auto forward_list_prepend(List * to) {
	return [to](auto node) { to->prepend(node); return forward_list_insert_after(to, node); };
}*/

} // ns detail

// list

template <c_forward_list_node Node, template <typename T1> typename Closer = closers::simple_one>
struct forward_list {
	using t_node = Node;
	using pointer = t_node *;
	using t_closer = Closer<t_node>;
	using t_iterator = detail::forward_list_iterator<t_node>;
	using t_sentinel = pointer;

	static constexpr bool is_node_custom = std::is_base_of_v<bases::forward_list_node<t_node>, t_node>;

	// data
	pointer head_ = nullptr, tail_ = nullptr;

	// no copy
	forward_list(const forward_list &) = delete;
	forward_list & operator = (const forward_list &) = delete;

	//
	forward_list() = default;
	~forward_list() { clear(); }

	// iteration
	t_iterator begin() const { return {head_}; }
	t_sentinel end() const { return nullptr; }

	operator bool () const { return head_; }
	bool operator ! () const { return !head_; }

	forward_list & clear() {
		while( head_ ) {
			pointer next = head_->next_;
			t_closer::close(head_);
			head_ = next;
		}
		tail_ = nullptr;
		return *this;
	}
	auto append(pointer node) {
		node->next_ = nullptr;
		if( tail_ ) tail_->next_ = node;
		else head_ = node;
		tail_ = node;
		return detail::forward_list_append(this);
	}
	auto prepend(pointer node) {
		if( !head_ ) tail_ = node;
		node->next_ = head_;
		head_ = node;
		return detail::forward_list_insert_after(this, node);
	}
	auto insert_after(pointer pos, pointer node) {
		if( pos ) {
			if( !pos->next_ ) tail_ = node;
			node->next_ = pos->next_;
			pos->next_ = node;
		} else prepend(node);
		return detail::forward_list_insert_after(this, node);
	}
	forward_list & remove_first() {
		if( head_ ) {
			pointer pos = head_;
			head_ = pos->next_;
			if( !head_ ) tail_ = nullptr;
			t_closer::close(pos);
		}
		return *this;
	}
	forward_list & remove_after(pointer pos) {
		if( pointer next = pos->next_ ) {
			pos->next_ = next->next_;
			if( !pos->next_ ) tail_ = pos;
			t_closer::close(next);
		}
		return *this;
	}
};

//

namespace detail {

template <typename T, typename Target = case_default>
struct forward_list_node {
	using pointer = std::conditional_t<std::is_same_v<Target, case_default>, forward_list_node *, Target *>;
	
	// data
	T value_;
	pointer next_;
};

template <typename T>
struct forward_list_node_cross :
	public forward_list_node<T, forward_list_node_cross<T> >,
	public bases::with_deleter< forward_list_node_cross<T> >
{};

template <typename T, bool Node_cross,
	template <c_forward_list_node Node1, template <typename T1> typename Closer1> typename List
>
using forward_list_alias = std::conditional_t<Node_cross,
	List< forward_list_node_cross<T>, closers::simple_call_deleter >,
	List< forward_list_node<T>, closers::simple_one >
>;

template <c_forward_list_node Node, template <typename T1> typename Closer>
struct forward_list_cross :
	public forward_list<Node, Closer>,
	public bases::with_deleter< forward_list_cross<Node, Closer> >
{
	using t_list = forward_list<Node, Closer>;
	using t_list::t_list;
};

} // ns detail

// simple & cross

template <typename T, bool List_cross = false, bool Node_cross = false>
using forward_list_alias = std::conditional_t<List_cross,
	detail::forward_list_alias<T, Node_cross, detail::forward_list_cross>,
	detail::forward_list_alias<T, Node_cross, forward_list>
>;

// actions

template <typename List>
auto forward_list_append(List & to) {
	return [h = &to]<typename ... Params>(Params && ... args) {
		if constexpr( List::is_node_custom ) {
			h->append( new List::t_node{std::forward<Params>(args) ...} );
		} else {
			h->append( new List::t_node{ {std::forward<Params>(args) ...} } );
		}
		return forward_list_append(*h);
	};
}
template <typename List>
auto forward_list_insert_after(List & to, typename List::pointer pos) {
	return [h = &to, pos]<typename ... Params>(Params && ... args) {
		typename List::pointer node;
		if constexpr( List::is_node_custom ) {
			h->insert_after( pos, node = new List::t_node{std::forward<Params>(args) ...} );
		} else {
			h->insert_after( pos, node = new List::t_node{ {std::forward<Params>(args) ...} } );
		}
		return forward_list_insert_after(*h, node);
	};
}
template <typename List>
auto forward_list_prepend(List & to) {
	return [h = &to]<typename ... Params>(Params && ... args) {
		typename List::pointer node;
		if constexpr( List::is_node_custom ) {
			h->prepend( node = new List::t_node{std::forward<Params>(args) ...} );
		} else {
			h->prepend( node = new List::t_node{ {std::forward<Params>(args) ...} } );
		}
		return forward_list_insert_after(*h, node);
	};
}

} // ns just