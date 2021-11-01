#pragma once

// Global defines. No don't complain about using unsecure functions
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
// Include M_PI and other defines
#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif

#define IR_SIZE 32

// C++ headers
#include <cassert>
#include <cstdio>
#include <cmath>
#include <array>
#include <vector>
#include <map>
#include <deque>
#include <bitset>
#include <algorithm>
#include <string>
#include <variant>
#include <sstream>

// Reduce the amount of symbols/headers when we include the windows.h
#define WIN32_LEAN_AND_MEAN
// Windows defines a macro min/max as define that interfieres with std::min/max
#define NOMINMAX
// Other macros to remove windows stuff
#define NODRAWTEXT
#define NOGDI
#define NOBITMAP
#define NOMCX
#define NOSERVICE
#define NOHELP
#include <Windows.h>
#include <mmsystem.h>

// External headers
#include "profiling/Remotery.h"
#include "imgui/imgui.h"
#include "utils/json.hpp"
using json = nlohmann::json;

// App Headers
#include "profiling/profiling.h"
#include "windows/application.h"
#include "geometry/geometry.h"
#include "utils/utils.h"
#include "resources/resources.h"
#include "render/render.h"
#include "handle/handle.h"
