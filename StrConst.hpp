#pragma once

#include <cstdlib>
#include <string>

using namespace std::string_literals;
#define C_STRING(name, value) struct name { static constexpr const char c_str[] = value; };
