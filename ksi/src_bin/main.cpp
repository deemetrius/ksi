import just.text;
import just.array;
import <iostream>;

int main() {
	{
		using namespace just::text_literals;
		just::wtext tx = L"hello"_jt;
		std::wcout << tx << L"\n";
	}{
		just::array<bool, just::capacity_step<8, 4> > arr;
		std::wcout << arr->count_ << L"\n";
		for( bool it : arr.get_reverse_range() ) std::wcout << it << L"\n";
	}
}