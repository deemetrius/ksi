import just.text;
import just.array;
import <iostream>;

int main() {
	{
		using namespace just::text_literals;
		just::wtext tx = L"hello"_jt;
		std::wcout << tx << L"\n";
	}{
		just::array<bool, just::capacity_step<3, 4> > arr;
		{ const just::id n = 2; new( just::array_append_n(arr, n) ) bool[n]{true, false}; }
		{ const just::id n = 2; new( just::array_insert_n(arr, 1, n) ) bool[n]{true}; }
		for( bool it : arr->get_reverse_range() ) std::wcout << it << L"\n";
		std::wcout << L"\t" << arr->capacity_ << L"\n";
	}
}