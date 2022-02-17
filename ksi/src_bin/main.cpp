import just.text.actions;
import just.compare;
import just.dict;
import just.keeper;
import just.forward_list;
import just.list;
import <compare>;
import <iostream>;

template <typename Array>
void print_items(const Array & arr) {
	for( auto it : arr ) std::wcout << it << L" ";
	std::wcout << L"\n";
}

template <typename Array>
void print_dict(const Array & arr) {
	for( const auto & it : arr->get_range() ) std::wcout << it.key_ << L": " << it.value_ << L", ";
	std::wcout << L"\n";
}

struct hold_base {
	virtual just::wtext name() const { using namespace just::text_literals; return L"hold_base"_jt; }
	//virtual ~hold_base() = default;
};
template <typename T, just::fixed_string Name>
struct hold : public hold_base {
	T value;
	hold(const T & v = T{}) : value(v) {}
	just::wtext name() const override {
		//return &just::detail::static_data<Name>::value;
		return just::text_literals::operator "" _jt<Name> ();
	}
};
struct b1 { bool x; using tt = hold<b1, L"b1">; };
struct b2 : public b1 { bool y; using tt = hold<b2, L"b2">; };
struct b3 : public b2 { bool z; using tt = hold<b3, L"b3">; ~b3() = default; };

struct point /*: public just::bases::forward_list_node<point>*/ { int x, y; point(int px = 0, int py = 0) : x(px), y(py) {} };

int main() {
	using namespace just::text_literals;
	{
		std::cout << "test compare: " << just::sign<int>(1.0 <=> 0.0/0.0, 2) << "\n";
	}{
		just::wtext tx = L"hello"_jt;
		just::text tx2 = "1"_jt;
		std::wcout << L"test text: " << (just::text_no_case{tx} == L"hEllo"_jt ? L'y' : L'n') << L"\n";
	}{
		just::array_alias<bool, just::capacity_step<3, 4> > arr;
		{ just::array_append_guard ag(arr, 2); new( ag.place ) bool[ag.quantity]{true, false}; }
		print_items(arr);
		{ just::array_insert_guard ag(arr, 1, 2); new( ag->place ) bool[ag->quantity]{true}; }
		print_items(arr);
		just::array_remove_last_n(arr, 1);
		print_items(arr);
		std::wcout << L"array capacity = " << arr->capacity_ << L"\n";
	}{
		using t_dict = just::dict< just::id, just::wtext, just::capacity_step<4, 4> >;
		t_dict::t_internal dict;
		t_dict::add(dict, 10, L"ten"_jt);
		t_dict::add(dict, 1, L"one"_jt);
		print_dict(dict);
		std::wcout << L"dict capacity = " << dict->capacity_ << L"\n" << t_dict::contains(dict, 1) << L"\n";
	}{
		using t_keep = just::keeper<hold_base, b1::tt, b2::tt, b3::tt>::t_internal;
		t_keep keep;
		keep.assign( new(&keep.place) b3::tt({true, false, true}) );
		std::wcout << keep->name() /*<< ".x = " << keep->x*/ << " " << keep.is_special << "\n";
	}{
		using t_list = just::forward_list_alias<point/*, false, just::case_none*/>;
		std::cout << "~list() triviality: " << std::is_trivially_destructible_v<t_list> << "\n";
		//using t_list = just::forward_list<point>;
		t_list lst;
		lst.prepend( new t_list::t_node{-2} )( new t_list::t_node{-1} );
		lst.append( new t_list::t_node{} )( new t_list::t_node{1} )( new t_list::t_node{point{2, 2}} );
		lst.insert_after( lst.tail_, new t_list::t_node{3} )( new t_list::t_node{4} );
		just::forward_list_insert_after(lst, lst.head_)()(10, 10)(20, 20);
		//just::forward_list_insert_after(lst, lst.tail_)(0)(10, 20)(5, 5)();
		for( t_list::pointer it : lst ) { std::cout << it->value_.x << ":" << it->value_.y << " "; }
		std::cout << "\n";
	}{
		using t_list = just::list_alias<point>;
		t_list lst;
		lst.prepend( new t_list::t_node{-2} )( new t_list::t_node{-1} );
		lst.append( new t_list::t_node{} )( new t_list::t_node{1} )( new t_list::t_node{point{2, 2}} );
		lst.insert_before( lst.tail_, new t_list::t_node{2} )( new t_list::t_node{{2, 1}} );
		just::list_insert_after(lst, lst.tail_)(0)(10, 20)(5, 5)();
		for( t_list::pointer it : lst.get_reverse_range() ) { std::cout << it->value_.x << ":" << it->value_.y << " "; }
		std::cout << "\n";
	}
	return 0;
}