import just.text.actions;
//import just.array;
import just.dict;
import <iostream>;

template <typename Array>
void print_items(const Array & arr) {
	for( auto it : arr->get_range() ) std::wcout << it << L" ";
	std::wcout << L"\n";
}

template <typename Array>
void print_dict(const Array & arr) {
	for( const auto & it : arr->get_range() ) std::wcout << it.key_ << L": " << it.value_ << L", ";
	std::wcout << L"\n";
}

int main() {
	using namespace just::text_literals;
	{
		just::wtext tx = L"hello"_jt;
		just::text tx2 = "1"_jt;
		std::wcout << (just::text_no_case{tx} == L"hEllo"_jt ? L'y' : L'n') << L"\n";
	}{
		just::array<bool, just::capacity_step<3, 4> > arr;
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
		std::wcout << L"dict capacity = " << dict->capacity_ << L"\n";
	}
	return 0;
}