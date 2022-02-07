module;

export module just.forward_list;
export import just.aux;
import <concepts>;
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

} // ns detail

// list

template <c_forward_list_node Node, template <typename T1> typename Closer = closers::simple_one>
struct forward_list {
	using t_node = Node;
	using pointer = t_node *;
	using t_closer = Closer<t_node>;
	using t_iterator = detail::forward_list_iterator<t_node>;

	// data
	pointer head_ = nullptr, last_ = nullptr;

	// no copy
	forward_list(const forward_list &) = delete;
	forward_list & operator = (const forward_list &) = delete;

	//
	forward_list() = default;
	~forward_list() { clear(); }

	// iteration
	t_iterator begin() const { return {head_}; }
	pointer end() const { return nullptr; }

	operator bool () const { return head_; }
	bool operator ! () const { return !head_; }

	forward_list & clear() {
		pointer next;
		while( head_ ) {
			next = head_->next_;
			t_closer::close(head_);
			head_ = next;
		}
		last_ = nullptr;
		return *this;
	}
	forward_list & append(pointer node) {
		node->next_ = nullptr;
		if( last_ ) last_->next_ = node;
		else head_ = node;
		last_ = node;
		return *this;
	}
	forward_list & prepend(pointer node) {
		if( !head_ ) last_ = node;
		node->next_ = head_;
		head_ = node;
		return *this;
	}
	forward_list & insert_after(pointer pos, pointer node) {
		if( !pos->next_ ) last_ = node;
		node->next_ = pos->next_;
		pos->next_ = node;
		return *this;
	}
	forward_list & remove_first() {
		if( head_ ) {
			pointer pos = head_;
			head_ = pos->next_;
			if( !head_ ) last_ = nullptr;
			t_closer::close(pos);
		}
		return *this;
	}
	forward_list & remove_after(pointer pos) {
		if( pointer next = pos->next_ ) {
			pos->next_ = next->next_;
			if( !pos->next_ ) last_ = pos;
			t_closer::close(next);
		}
		return *this;
	}
};

//

namespace detail {

template <typename T, typename Target = void>
struct forward_list_node {
	using pointer = std::conditional_t<std::is_same_v<Target, void>, forward_list_node *, Target *>;
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
	return [to = &to]<typename ... Params>(Params && ... args) {
		to->append( new List::t_node{ {std::forward<Params>(args) ...} } );
		return forward_list_append(*to);
	};
}

template <typename List>
auto forward_list_prepend(List & to) {
	return [to = &to]<typename ... Params>(Params && ... args) {
		to->prepend( new List::t_node{ {std::forward<Params>(args) ...} } );
		return forward_list_prepend(*to);
	};
}

template <typename List>
auto forward_list_insert_after(List & to, typename List::pointer pos) {
	return [to = &to, pos]<typename ... Params>(Params && ... args) {
		to->insert_after( new List::t_node{ {std::forward<Params>(args) ...} } );
		return forward_list_insert_after(*to, pos);
	};
}

} // ns just