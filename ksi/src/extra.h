#pragma once
#include <initializer_list>
#include <cstring>
#include <cwchar>
#include <iostream>

namespace ex {

using id = signed long long int;
using ref_id = id;
using uid = unsigned long long int;
using real = double;
#define WCS_TO_REAL wcstod

template <class T>
void swap(T & v1, T & v2) {
	T tmp = v1;
	v1 = v2;
	v2 = tmp;
}

// with_deleter

template <class Target>
struct with_deleter {
	using hfn_deleter = void (*)(const Target * h);
	hfn_deleter deleter_ = deleter;

	static void deleter(const Target * h) {
		delete h;
	}
};

// mem
struct mem_new {
	using fn_de_allocate = void (*)(char * items);
	using fn_re_allocate = char * (*)(fn_de_allocate fn_unset, char * items, id item_size, id new_count, id count_copy);
	static char * allocate(id item_size, id count) {
		return new char[item_size * count];
	}
	static char * f_re_allocate(fn_de_allocate fn_unset, char * items, id item_size, id new_count, id count_copy) {
		char * new_items = new char[item_size * new_count];
		if( count_copy )
		std::memcpy(new_items, items, item_size * count_copy);
		//de_allocate(items);
		fn_unset(items);
		return new_items;
	}
	static fn_re_allocate re_allocate;
	static void de_allocate(char * items) {
		delete [] items;
	}
};

// closer plain
template <class T>
struct del_plain {
	using pass = T;

	static void del_one(T & item) {}

	template <class Hive>
	static void del_many(Hive * hive, id till) {}

	static void set_init(T * item, pass val) {
		*item = val;
	}
};

// closer plain struct
template <class T>
struct del_plain_struct {
	using pass = const T &;

	static void del_one(T & item) {}

	template <class Hive>
	static void del_many(Hive * hive, id till) {}

	static void set_init(T * item, pass val) {
		*item = val;
	}
};


// closer object
template <class T>
struct del_object {
	using pass = const T &;

	static void del_one(T & item) {
		item.~T();
	}

	template <class Hive>
	static void del_many(Hive * hive, id till) {
		for( T & item : hive->get_rev_iter(till) )
		del_one(item);
	}

	static void set_init(T * item, pass val) {
		new(item) T(val);
	}
};

// closer pointer
template <class T>
struct del_pointer {
	using pass = T;

	static void del_one(T & item) {
		delete item;
	}

	template <class Hive>
	static void del_many(Hive * hive, id till) {
		for( T & item : hive->get_rev_iter(till) )
		del_one(item);
	}

	static void set_init(T * item, T val) {
		*item = val;
	}
};

// closer ex_pointer (of struct with deleter_)
template <class T>
struct del_ex_pointer {
	using pass = T;

	static void del_one(T & item) {
		item->deleter_(item);
	}

	template <class Hive>
	static void del_many(Hive * hive, id till) {
		for( T & item : hive->get_rev_iter(till) )
		del_one(item);
	}

	static void set_init(T * item, T val) {
		*item = val;
	}
};

// reverse iterator
template <class T>
struct rev_iter {
	T * begin_, * end_;
	void operator ++ () {
		end_ -= 1;
	}
	T & operator * () {
		return *end_;
	}
	bool operator != (T * the_end) const {
		return end_ != the_end;
	}
	rev_iter & begin() {
		return *this;
	}
	T * end() {
		return begin_;
	}
};

// array
template <class T, template <class Item> class Closer, class Mem = mem_new>
struct array {
	using pass = typename Closer<T>::pass;
	T * items_;
	id count_, size_, step_;
	//char * (* allocate_)(id, id) = Mem::allocate;
	//char * (* re_allocate_)(char *, id, id, id) = Mem::re_allocate;
	void (* de_allocate_)(char *) = Mem::de_allocate;

	array(const array &) = delete;
	array & operator = (const array &) = delete;

	array & clear() {
		Closer<T>::del_many(this, 0);
		count_ = 0;
		return *this;
	}

	array(id reserve, id step) : count_(0), size_(reserve), step_(step) {
		items_ = reinterpret_cast<T *>( Mem::allocate(sizeof(T), size_) );
	}
	~array() {
		Closer<T>::del_many(this, 0);
		de_allocate_( reinterpret_cast<char *>(items_) );
	}

	T * begin() const { return items_; }
	T * end() const { return items_ + count_; }

	T * rbegin() const { return  items_ + count_ -1; }
	T * rend() const { return items_ -1; }

	T & last(id num) {
		return items_[count_ -1 - num];
	}
	rev_iter<T> get_rev_iter(id num = 0) {
		return rev_iter<T>{items_ + num -1, items_ + count_ -1};
	}
	void more(id amount) {
		items_ = reinterpret_cast<T *>(
			Mem::re_allocate(de_allocate_, reinterpret_cast<char *>(items_), sizeof(T), amount, count_)
		);
		de_allocate_ = Mem::de_allocate;
		size_ = amount;
	}
	void prepare_append() {
		if( count_ +1 > size_ )
		more(size_ + step_);
	}
	array & append(pass item) {
		prepare_append();
		Closer<T>::set_init(items_ + count_, item);
		count_ += 1;
		return *this;
	}
	template <class ... Args>
	array & append_obj(Args ... args) {
		prepare_append();
		new(items_ + count_) T(args ...);
		count_ += 1;
		return *this;
	}
	template <class ... Args>
	struct detail {
		using t_mf = array & (array::*)(Args ...);

		static constexpr t_mf get_append_obj() {
			return &append_obj<Args ...>;
		}
	};
	template <class ... Args>
	array & append_struct(Args ... args) {
		prepare_append();
		new(items_ + count_) T{args ...};
		count_ += 1;
		return *this;
	}
	array & remove(id pos) {
		Closer<T>::del_one(items_[pos]);
		id new_count = count_ -1;
		if( pos < new_count )
		std::memmove(reinterpret_cast<char *>(items_ + pos), items_ + pos +1, sizeof(T) * (new_count - pos) );
		count_ = new_count;
		return *this;
	}
	array & remove_from_end(id pos_from_end) {
		remove(count_ - pos_from_end -1);
		return *this;
	}
	array & remove_last_n(id n) {
		id till = count_ - n;
		Closer<T>::del_many(this, till);
		count_ = till;
		return *this;
	}
	void prepare_insert(id pos) {
		if( count_ +1 > size_ ) {
			id new_size = size_ + step_;
			T * items = reinterpret_cast<T *>( Mem::allocate(sizeof(T), new_size) );
			if( pos > 0 )
			std::memcpy(reinterpret_cast<char *>(items), items_, sizeof(T) * pos);
			std::memcpy(reinterpret_cast<char *>(items + pos +1), items_ + pos, sizeof(T) * (count_ - pos) );
			de_allocate_( reinterpret_cast<char *>(items_) );
			de_allocate_ = Mem::de_allocate;
			items_ = items;
			size_ = new_size;
		} else
		std::memmove(reinterpret_cast<char *>(items_ + pos +1), items_ + pos, sizeof(T) * (count_ - pos) );
	}
	array & insert(id pos, pass item) {
		if( pos >= count_ )
		append(item);
		else {
			prepare_insert(pos);
			Closer<T>::set_init(items_ + pos, item);
			count_ += 1;
		}
		return * this;
	}
	template <class ... Args>
	array & insert_obj(id pos, Args ... args) {
		if( pos >= count_ )
		append_obj(args ...);
		else {
			prepare_insert(pos);
			new(items_ + pos) T(args ...);
			count_ += 1;
		}
		return * this;
	}
	template <class ... Args>
	array & insert_struct(id pos, Args ... args) {
		if( pos >= count_ )
		append_struct(args ...);
		else {
			prepare_insert(pos);
			new(items_ + pos) T{args ...};
			count_ += 1;
		}
		return * this;
	}
	bool empty() {
		return count_ == 0;
	}
};

template <class T, template <class Item> class Closer, id Reserve, id Step>
struct def_array : public array<T, Closer> {
	using base = array<T, Closer>;
	def_array() : base(Reserve, Step) {}
};

// list

struct node {
	node * prev_, * next_;
	template <class List>
	typename List::cnode * get(const List & lst) {
		return &lst.zero_ == this ? nullptr : static_cast<typename List::cnode *>(this);
	}
};

template <class T>
struct map_del_plain {
	using pass = T;
	static void close(const T & item) {}
};

template <class T>
struct map_del_object {
	using pass = const T &;
	static void close(const T & item) {}
};

template <class T>
struct map_del_pointer {
	using pass = T;
	static void close(const T & item) {
		delete item;
	}
};

template <class T>
struct map_del_ex_pointer {
	using pass = T;
	static void close(const T & item) {
		item->deleter_(item);
	}
};

template <bool IsRev, class Node>
struct list_iter {
	node * end_, * cur_, * next_;
	list_iter & begin() {
		return *this;
	}
	node * end() {
		return end_;
	}
	void operator ++ () {
		cur_ = next_;
		if constexpr( IsRev )
		next_ = cur_->prev_;
		else
		next_ = cur_->next_;
	}
	Node * operator * () {
		return static_cast<Node *>(cur_);
	}
	bool operator != (node * the_end) const {
		return cur_ != the_end;
	}
};

template <class Key, class Val, template <class K> class DelKey, template <class V> class DelVal>
struct list {
	using pass_key = typename DelKey<Key>::pass;
	using pass_val = typename DelVal<Val>::pass;
	struct pair {
		Key key_;
		Val val_;
		~pair() {
			DelKey<Key>::close(key_);
			DelVal<Val>::close(val_);
		}
	};
	struct cnode : public pair, public node, public with_deleter<cnode> {};
	node zero_;

	list(const list &) = delete;
	list & operator = (const list &) = delete;

	list & clear() {
		for( cnode * item : get_rev_iter() )
		item->deleter_(item);
		zero_.next_ = &zero_;
		zero_.prev_ = &zero_;
		return *this;
	}

	list() : zero_{&zero_, &zero_} {}
	~list() {
		for( cnode * item : get_rev_iter() )
		item->deleter_(item);
	}

	list_iter<false, cnode> begin() {
		return {&zero_, zero_.next_, zero_.next_->next_};
	}
	node * end() {
		return &zero_;
	}

	list_iter<true, cnode> get_rev_iter() {
		return {&zero_, zero_.prev_, zero_.prev_->prev_};
	}

	list & insert_after(cnode * item, node * after) {
		node * next = after->next_;
		item->prev_ = after;
		item->next_ = next;
		next->prev_ = item;
		after->next_ = item;
		return *this;
	}
	list & append(pass_key key, pass_val val) {
		cnode * item = new cnode{key, val};
		insert_after(item, zero_.prev_);
		return *this;
	}
	list & prepend(pass_key key, pass_val val) {
		cnode * item = new cnode{key, val};
		insert_after(item, &zero_);
		return *this;
	}
	list & append(cnode * item) {
		insert_after(item, zero_.prev_);
		return *this;
	}
	list & prepend(cnode * item) {
		insert_after(item, &zero_);
		return *this;
	}
	list & detach(node * item, bool del) {
		if( item != &zero_ ) {
			item->next_->prev_ = item->prev_;
			item->prev_->next_ = item->next_;
			if( del ) {
				cnode * h = static_cast<cnode *>(item);
				h->deleter_(h);
			}
		}
		return *this;
	}
	cnode * get(node * item) {
		return (item == &zero_) ? nullptr : static_cast<cnode *>(item);
	}
};

// map

enum class same_key { ignore, update, reorder };

template <class T>
struct search_res {
	bool found_;
	T pos_;
	inline operator bool () const {
		return found_;
	}
};
using id_search_res = search_res<id>;

struct cmp {
	enum t_res : id { less = -1, equal = 0, more = +1 };
	template <class T1, class T2>
	static id special_compare(T1 v1, T2 v2) {
		return v1 < v2 ? cmp::less : cmp::more;
	}
};

template <class Key, class Val>
struct pair {
	Key key_;
	Val val_;
};

template <
	class Key,
	class Val,
	template <class K> class DelKey,
	template <class V> class DelVal,
	class KeyCompare
>
struct map {
	using t_res = id_search_res;
	using pass_key = typename DelKey<Key>::pass;
	using pass_val = typename DelVal<Val>::pass;
	using t_items = list<const Key, Val, DelKey, DelVal>;
	using cnode = typename t_items::cnode;
	using t_res_node = search_res<cnode *>;
	using t_sorted = array<cnode *, del_plain>;
	t_items items_;
	t_sorted sorted_;

	map(const map & m, id extra) = delete;
	map & operator = (const map &) = delete;

	map & clear() {
		sorted_.clear();
		items_.clear();
		return *this;
	}

	map(id reserve, id step) : sorted_(reserve, step) {}
	map(id step, same_key sk, std::initializer_list< pair<Key, Val> > lst) : sorted_(lst.size(), step) {
		for( const pair<Key, Val> & it : lst )
		append(it.key_, it.val_, sk);
	}
	t_res find_key(pass_key key) const {
		switch( sorted_.count_ ) {
		case 0:
			return {false, 0}; break;
		case 1:
			switch( KeyCompare::compare(key, sorted_.items_[0]->key_) ) {
			case cmp::less :
				return {false, 0}; break;
			case cmp::equal :
				return {true, 0}; break;
			default:
				return {false, 1}; break;
			}
			break;
		}
		// check last
		id right = sorted_.count_ -1;
		switch( KeyCompare::compare(key, sorted_.items_[right]->key_) ) {
		case cmp::equal :
			return {true, right}; break;
		case cmp::more :
			return {false, sorted_.count_}; break;
		}
		right -= 1;
		// check first
		switch( KeyCompare::compare(key, sorted_.items_[0]->key_) ) {
		case cmp::less :
			return {false, 0}; break;
		case cmp::equal :
			return {true, 0}; break;
		}
		id left = 1;
		// count is 2
		if( left > right )
		return {false, left};
		// go mid search
		id mid, comp;
		do {
			mid = (left + right) / 2;
			switch( comp = KeyCompare::compare(key, sorted_.items_[mid]->key_) ) {
			case cmp::equal :
				return {true, mid}; break;
			case cmp::less :
				right = mid - 1;
				break;
			default:
				left = mid + 1;
			}
		} while( left <= right );
		return {false, mid + (comp > 0 ? 1 : 0)};
	}
	t_res_node find_node(pass_key key) const {
		if( t_res res = find_key(key) )
		return {true, sorted_.items_[ res.pos_ ]};

		return {false};
	}
	inline node * in_begin() {
		return &items_.zero_;
	}
	inline node * in_end() {
		return items_.zero_.prev_;
	}
	cnode * inner_insert_after(pass_key key, pass_val val, same_key sk, node * after, const t_res & res) {
		cnode * item;
		if( res ) {
			item = sorted_.items_[res.pos_];
			if( sk == same_key::ignore ) {
				DelVal<Val>::close(val);
			} else {
				DelVal<Val>::close(item->val_);
				item->val_ = val;
				if( sk == same_key::reorder )
				items_.detach(item, false).insert_after(item, after);
			}
		} else {
			item = new cnode{key, val};
			sorted_.insert(res.pos_, item);
			items_.insert_after(item, after);
		}
		return item;
	}
	cnode * insert_after(pass_key key, pass_val val, same_key sk, node * after) {
		return inner_insert_after(key, val, sk, after, find_key(key) );
	}
	cnode * append(pass_key key, pass_val val, same_key sk) {
		return insert_after(key, val, sk, in_end() );
	}
	cnode * prepend(pass_key key, pass_val val, same_key sk) {
		return insert_after(key, val, sk, in_begin() );
	}
	template <class V>
	cnode * v_inner_insert_after(pass_key key, V val, same_key sk, node * after, const t_res & res) {
		cnode * item;
		if( res ) {
			item = sorted_.items_[res.pos_];
			if( sk != same_key::ignore ) {
				DelVal<Val>::close(item->val_);
				item->val_ = val;
				if( sk == same_key::reorder )
				items_.detach(item, false).insert_after(item, after);
			}
		} else {
			item = new cnode{key, val};
			sorted_.insert(res.pos_, item);
			items_.insert_after(item, after);
		}
		return item;
	}
	template <class V>
	cnode * v_insert_after(pass_key key, V val, same_key sk, node * after) {
		return v_inner_insert_after<V>(key, val, sk, after, find_key(key) );
	}
	template <class V>
	cnode * v_append(pass_key key, V val, same_key sk) {
		return v_insert_after<V>(key, val, sk, in_end() );
	}
	template <class V>
	cnode * v_prepend(pass_key key, V val, same_key sk) {
		return v_insert_after<V>(key, val, sk, in_begin() );
	}
	map & detach(pass_key key) {
		if( t_res res = find_key(key) ) {
			cnode * item = sorted_.items_[res.pos_];
			sorted_.remove(res.pos_);
			items_.detach(item, true);
		}
		return *this;
	}
};

template <
	class Key,
	class Val,
	template <class K> class DelKey,
	template <class V> class DelVal,
	class KeyCompare,
	id Reserve,
	id Step
>
struct def_map : public map<Key, Val, DelKey, DelVal, KeyCompare> {
	using base = map<Key, Val, DelKey, DelVal, KeyCompare>;
	def_map() : base(Reserve, Step) {}
};

// wtext

template <class T>
struct traits_text;

template <>
struct traits_text<wchar_t> {
	using Char = wchar_t;
	static constexpr Char v_empty[] = L"";
	inline static id length(const Char * cs) {
		return std::wcslen(cs);
	}
	inline static Char * copy_n(Char * dest, const Char * src, id count) {
		return std::wcsncpy(dest, src, count);
	}
	inline static id compare(const Char * s1, const Char * s2) {
		return std::wcscmp(s1, s2);
	}
};

template <>
struct traits_text<char> {
	using Char = char;
	static constexpr Char v_empty[] = "";
	inline static id length(const Char * cs) {
		return std::strlen(cs);
	}
	inline static Char * copy_n(Char * dest, const Char * src, id count) {
		return std::strncpy(dest, src, count);
	}
	inline static id compare(const Char * s1, const Char * s2) {
		return std::strcmp(s1, s2);
	}
};

struct base_text {
	enum class n_copy { val };
	enum class n_get { val };
};

template <class T, template <class T1> class Traits = traits_text>
struct basic_text : public base_text {
	using Char = T;
	using traits = Traits<T>;
	struct keep : public with_deleter<keep> {
		const Char * cs_;
		Char * s_;
		id len_;
		ref_id refs_ = 1;

		keep() : cs_(traits::v_empty), s_(nullptr), len_(0) {}
		keep(Char * s, id len) : cs_(s), s_(s), len_(len) {}
		template <id N>
		keep(const Char (& cs)[N]) : cs_(cs), s_(nullptr), len_(N -1) {}
		keep(n_copy, const Char * cs, id len) : len_(len) {
			s_ = new Char[len +1];
			if( len )
			traits::copy_n(s_, cs, len);
			s_[len] = 0;
			cs_ = s_;
		}
		keep(n_get, const Char * cs) : cs_(cs), s_(nullptr) {
			calc_len();
		}
		~keep() {
			delete [] s_;
		}
		id calc_len() {
			return len_ = traits::length(cs_);
		}
	};
	keep * h_;

	void hold() const {
		h_->refs_ += 1;
	}
	void unhold() const {
		if( h_->refs_ <= 1 )
		h_->deleter_(h_);
		else
		h_->refs_ -= 1;
	}
	~basic_text() {
		unhold();
	}

	basic_text(const basic_text & tx) {
		tx.hold();
		h_ = tx.h_;
	}
	basic_text & operator = (const basic_text & tx) {
		tx.hold();
		unhold();
		h_ = tx.h_;
		return *this;
	}

	enum class n_empty { val };
	basic_text(n_empty) {
		h_ = new keep();
	}
	basic_text() : basic_text( inst_empty() ) {}
	static const basic_text & inst_empty() {
		static const basic_text tx(n_empty::val);
		return tx;
	}

	basic_text(Char * s, id len) {
		h_ = new keep(s, len);
	}
	template <id N>
	basic_text(const Char (& cs)[N]) {
		h_ = new keep(cs);
	}
	basic_text(const Char (&cs)[1]) : basic_text( inst_empty() ) {}

	template <id N>
	basic_text & operator = (const Char (& cs)[N]) {
		unhold();
		h_ = new keep(cs);
		return *this;
	}
	basic_text & operator = (const Char (&cs)[1]) {
		*this = inst_empty();
		return *this;
	}

	basic_text(n_copy v, const Char * cs, id len) {
		h_ = new keep(v, cs, len);
	}

	basic_text(n_get v, const Char * cs) {
		h_ = new keep(v, cs);
	}

	basic_text(const Char * begin, const Char * end) {
		h_ = new keep(n_copy::val, begin, end - begin);
	}
	basic_text(const basic_text & tx, const Char * begin, const Char * end) {
		id tx_len = tx.h_->len_, it_len = end - begin, len = tx_len + it_len;
		Char * str = new Char[len +1];
		h_ = new keep(str, len);
		if( tx_len ) {
			traits::copy_n(str, tx.h_->cs_, tx_len);
			str += tx_len;
		}
		if( it_len )
		traits::copy_n(str, begin, it_len);
		h_->s_[len] = 0;
	}

	basic_text(Char ch) {
		h_ = new keep(new Char[2], 1);
		h_->s_[0] = ch;
		h_->s_[1] = 0;
	}

	bool empty() const {
		return h_->len_ == 0;
	}
	operator bool () const {
		return h_->len_;
	}
};

using wtext = basic_text<wchar_t>;
using text = basic_text<char>;

wtext to_wtext(id num);
wtext to_wtext(real num);

struct traits {
	static id num_digits(id n) {
		id cnt;
		for( cnt = 1; n /= 10; ++cnt );
		return cnt;
	}
	static id num_len(id n) {
		return num_digits(n) + (n < 0);
	}

	struct s_counter {
		id cnt_ = 0;
		s_counter & operator << (const wtext & tx) {
			cnt_ += tx.h_->len_;
			return *this;
		}
		template <id N>
		s_counter & operator << (const wtext::Char (&cs)[N]) {
			cnt_ += N -1;
			return *this;
		}
		s_counter & operator << (wtext::Char) {
			cnt_ += 1;
			return *this;
		}
		s_counter & operator << (id n) {
			cnt_ += num_len(n);
			return *this;
		}
		s_counter & add(const wtext::Char * begin, const wtext::Char * end) {
			cnt_ += end - begin;
			return *this;
		}
	};
	struct s_concater {
		wtext tx_;
		wtext::Char * s_;
		s_concater(id len) : tx_(new wtext::Char[len +1], len) {
			s_ = tx_.h_->s_;
			s_[len] = 0;
		}
		s_concater & operator << (const wtext & tx) {
			if( tx.h_->len_ ) {
				wcsncpy(s_, tx.h_->cs_, tx.h_->len_);
				s_ += tx.h_->len_;
			}
			return *this;
		}
		template <id N>
		s_concater & operator << (const wtext::Char (&cs)[N]) {
			if constexpr( N > 1 ) {
				wcsncpy(s_, cs, N -1);
				s_ += N -1;
			}
			return *this;
		}
		s_concater & operator << (wtext::Char ch) {
			*s_ = ch;
			s_ += 1;
			return *this;
		}
		s_concater & operator << (id n) {
			return *this << to_wtext(n);
		}
		s_concater & add(const wtext::Char * begin, const wtext::Char * end) {
			if( id len = end - begin ) {
				wcsncpy(s_, begin, len);
				s_ += len;
			}
			return *this;
		}
	};

	template <class S>
	static void inner_escape(const wtext & tx, S & s, bool wrap_with_quotes = false) {
		if( wrap_with_quotes )
		s << L'"';
		const wtext::Char * str = tx.h_->cs_;
		while( *str != 0 ) {
			switch( *str ) {
			case L'\n':	s << L"\\n"; break;
			case L'\r':	s << L"\\r"; break;
			case L'\t':	s << L"\\t"; break;
			case L'\v':	s << L"\\v"; break;
			case 27:	s << L"\\e"; break;
			case L'\f':	s << L"\\f"; break;
			case L'"':	s << L"\\\""; break;
			case L'\\':	s << L"\\\\"; break;
			default:
				if( iswprint(*str) )
				s << *str;
				else
				s << L"\\u{" << static_cast<id>(*str) << L'}';
			}
			str += 1;
		}
		if( wrap_with_quotes )
		s << L'"';
	}
	static wtext escape_text(const wtext & tx, bool wrap_with_quotes = false) {
		s_counter s_cnt;
		inner_escape(tx, s_cnt, wrap_with_quotes);
		s_concater s_cat(s_cnt.cnt_);
		inner_escape(tx, s_cat, wrap_with_quotes);
		return s_cat.tx_;
	}

	static bool find_char(const wtext & tx, wtext::Char ch, id & pos) {
		for( pos = 0; pos < tx.h_->len_; ++pos )
		if( tx.h_->cs_[pos] == ch )
		return true;
		return false;
	}
	static wtext before_pos(const wtext & tx, id pos) {
		return wtext(tx.h_->cs_, tx.h_->cs_ + pos);
	}
	static wtext after_pos(const wtext & tx, id pos) {
		return wtext(tx.h_->cs_ + pos, tx.h_->cs_ + tx.h_->len_);
	}
};

using wtext_array = array<wtext, del_object>;
wtext implode(std::initializer_list<wtext> lst, const wtext & sep = L"");
wtext implode(const wtext_array & items, const wtext & sep = L"");
wtext replace_filename(const wtext & path, const wtext & file);
wtext absolute_path(const wtext & path);

inline std::wostream & operator << (std::wostream & wo, const wtext & tx) {
	return wo << tx.h_->cs_;
}

// ref

template <class T, bool IsArray>
struct ref {
	using fn_deleter = void (*)(const T *);
	mutable T * h_ = nullptr;
	mutable fn_deleter deleter_ = none_deleter;

	static void deleter(const T * h) {
		if constexpr( IsArray )
		delete [] h;
		else
		delete h;
	}
	static void none_deleter(const T *) {}

	ref() = default;
	ref(T * h) : h_(h), deleter_(deleter) {}
	~ref() {
		deleter_(h_);
	}

	void operator = (T * h) {
		deleter_(h_);
		h_ = h;
		deleter_ = deleter;
	}
	void operator = (std::nullptr_t) {
		close();
	}

	operator bool () const {
		return h_;
	}

	void detach() const {
		h_ = nullptr;
		deleter_ = none_deleter;
	}
	void close() const {
		deleter_(h_);
		detach();
	}

	void swap(const ref & r) const {
		swap(h_, r.h_);
		swap(deleter_, r.deleter_);
	}

	// copy
	ref(const ref & r) : h_(r.h_), deleter_(r.deleter_) {
		r.detach();
		//std::wcout << L" <ref_copy> ";
	}
	ref & operator = (const ref & r) {
		swap(r);
		return *this;
	}
};

//bool read_file(const wtext & path, ref<char, true> & r, id & len);
bool read_file(const wtext & path, text & r, id & len);

// cmp

struct cmp_std_plain : public cmp {
	template <class T1, class T2>
	static id compare(T1 v1, T2 v2) {
		return v1 < v2 ? less : (v1 == v2 ? equal : more);
	}

	template <class T>
	static id sign(T v) {
		return v < 0 ? less : (v == 0 ? equal : more);
	}

	static id compare(const wtext::Char * v1, const wtext::Char * v2) {
		return sign( std::wcscmp(v1, v2) );
	}
	static id compare(const wtext & v1, const wtext & v2) {
		return compare(v1.h_->cs_, v2.h_->cs_);
	}
};

template <class T, template <class T1> class Traits>
inline bool operator == (const basic_text<T, Traits> & t1, const basic_text<T, Traits> & t2) {
	return ( t1.h_->len_ == t2.h_->len_ ) ? !Traits<T>::compare(t1.h_->cs_, t2.h_->cs_) : false;
}

// hive

template <class T, template <class Item> class Closer, id Reserve, id Step>
struct hive {
	using t_arr = def_array<T, Closer, Reserve, Step>;
	using t_map = def_map<
		wtext, id, map_del_object, map_del_plain, cmp_std_plain,
		Reserve, Step
	>;
	using pass = typename t_arr::pass;
	using t_res = search_res<T>;
	t_arr arr_;
	t_map map_;

	id inner_add(const wtext & name, pass item, const id_search_res & res) {
		if( res ) return map_.sorted_.items_[res.pos_]->val_;
		id ret = arr_.count_;
		arr_.append(item);
		map_.inner_insert_after(name, ret, same_key::ignore, map_.in_end(), res);
		return ret;
	}
	inline id add(const wtext & name, pass item) {
		return inner_add(name, item, map_.find_key(name));
	}

	template <class ... Args>
	id inner_add_obj(const wtext & name, const id_search_res & res, Args ... args) {
		if( res ) return map_.sorted_.items_[res.pos_]->val_;
		id ret = arr_.count_;
		using det = typename t_arr::detail<Args ...>;
		static constexpr typename det::t_mf mf = det::get_append_obj();
		(arr_.*mf)(args ...);
		map_.inner_insert_after(name, ret, same_key::ignore, map_.in_end(), res);
		return ret;
	}
	template <class ... Args>
	inline id add_obj(const wtext & name, Args ... args) {
		return inner_add_obj<Args ...>(name, map_.find_key(name), args ...);
	}

	id_search_res find_pos(const wtext & name) {
		if( id_search_res res = map_.find_key(name) )
		return {true, map_.sorted_.items_[res.pos_]->val_};
		else
		return {false};
	}
	t_res find_item(const wtext & name) {
		if( id_search_res res = find_pos(name) )
		return {true, arr_.items_[res.pos_]};
		else
		return {false};
	}

	inline T & get_by_pos(id pos) {
		return arr_.items_[pos];
	}
};

} // ns
