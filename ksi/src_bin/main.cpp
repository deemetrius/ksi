import just.text.actions;
import just.array;
import <iostream>;

int main() {
	{
		using namespace just::text_literals;
		just::wtext tx = L"hello"_jt;
		std::wcout << tx << L"\n";
		std::wcout << (just::text_no_case{tx} == L"hEllo"_jt ? L'y' : L'n') << L"\n";
	}{
		just::array<bool, just::capacity_step<3, 4> > arr;
		{ const just::id n = 2; new( just::array_append_n(arr, n) ) bool[n]{true, false}; }
		{ const just::id n = 2; new( just::array_insert_n(arr, 1, n) ) bool[n]{true}; }
		just::array_remove_last_n(arr, 1);
		for( bool it : arr->get_range() ) std::wcout << it << L" ";
		std::wcout << L"\n" << arr->capacity_ << L"\n";
	}
}