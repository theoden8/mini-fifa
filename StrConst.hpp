#pragma once

#include <cstdlib>

#define C_STRING(name, value) struct name { static constexpr const char *c_str = value; };
