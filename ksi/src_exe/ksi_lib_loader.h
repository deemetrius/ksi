#pragma once
#include "../src/ksi_api.h"

namespace ksi {

struct lib_loader {
	static const api * load_and_get_api();
};

} // ns
