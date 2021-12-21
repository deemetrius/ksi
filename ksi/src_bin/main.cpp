import just.text;
import <iostream>;

int main() {
	{
		using namespace just::text_literals;
		just::wtext tx = L"hello"_jt;
		std::wcout << tx << "\n";
	}
}