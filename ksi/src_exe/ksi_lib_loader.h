#pragma once
#include "../src/ksi_api.h"

namespace ksi {

struct lib_loader {
	static const api * load_and_get_api(const ex::wtext & file = L"");
	static ex::wtext calc_path(const ex::wtext & file);
};

} // ns
