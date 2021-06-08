#pragma once
#include "ksi_types.h"
#include "ksi_api.h"

namespace ksi {

extern std::wostream * wc;

#ifdef KSI_LIB
const api * get_api();
bool load_extension(const wtext & path);
bool run_script(const wtext & path, const run_args & ra, base_log * log);
#endif // KSI_LIB

} // ns
