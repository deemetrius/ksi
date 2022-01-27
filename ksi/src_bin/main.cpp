import just.text.actions;
import just.array;
import <iostream>;

template <typename Array>
void print_items(const Array & arr) {
	for( bool it : arr->get_range() ) std::wcout << it << L" ";
	std::wcout << L"\n";
}

int main() {
	{
		using namespace just::text_literals;
		just::wtext tx = L"hello"_jt;
		//just::text tx2 = "1"_jt;
		std::wcout << (just::text_no_case{tx} == L"hEllo"_jt ? L'y' : L'n') << L"\n";
	}{
		just::array<bool, just::capacity_step<3, 4> > arr;
		//{ const just::id n = 2; new( just::array_append_n(arr, n) ) bool[n]{true, false}; }
		//{ const just::id n = 2; new( just::array_insert_n(arr, 1, n) ) bool[n]{true}; }
		{ just::array_append_guard ag(arr, 2); new( ag.place ) bool[ag.quantity]{true, false}; }
		print_items(arr);
		{ just::array_insert_guard ag(arr, 1, 2); new( ag->place ) bool[ag->quantity]{true}; }
		print_items(arr);
		just::array_remove_last_n(arr, 1);
		print_items(arr);
		std::wcout << arr->capacity_ << L"\n";
	}
	return 0;
}