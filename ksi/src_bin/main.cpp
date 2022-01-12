module;

export module try_main;
import just.text.actions;
import just.array;
import <iostream>;

int main() {
	{
		using namespace just::text_literals;
		just::wtext tx = L"hello"_jt;
		just::text tx2 = "1"_jt;
		//std::wcout << tx->cs_ << L"\n";
		//std::wcout << (just::text_no_case{tx} == L"hEllo"_jt ? L'y' : L'n') << L"\n";
		//just::wtext tx2 = L"123"_jt;
		//tx2 = tx;
		//tx = just::text_implode({tx, L" ;"_jt});
	}{
		just::array<bool, just::capacity_step<3, 4> > arr;
		{ const just::id n = 2; new( just::array_append_n(arr, n) ) bool[n]{true, false}; }
		{ const just::id n = 2; new( just::array_insert_n(arr, 1, n) ) bool[n]{true}; }
		just::array_remove_last_n(arr, 1);
		for( bool it : arr->get_range() ) std::wcout << it << L" ";
		std::wcout << L"\n" << arr->capacity_ << L"\n";
	}
	return 0;
}