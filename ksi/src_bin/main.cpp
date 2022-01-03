import just.text;
import just.array;
import <iostream>;

int main() {
	{
		using namespace just::text_literals;
		just::wtext tx = L"hello"_jt;
		std::wcout << tx << L"\n";
	}{
		just::array_pod<bool, just::capacity_step<8, 4> > arr;
		{ const just::id n = 2; new( just::append_n(arr, 2) ) bool[n]{true, false}; }
		//std::wcout << arr->count_ << L"\n";
		for( bool it : arr.get_range() ) std::wcout << it << L"\n";
	}
}