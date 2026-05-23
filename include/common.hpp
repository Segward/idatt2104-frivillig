#ifndef COMMON_HPP
#define COMMON_HPP

// Project-wide precompiled header. Registered in the top-level CMakeLists.txt
// via target_precompile_headers(... PUBLIC ...), so every translation unit
// in crdt, server, and tests sees these includes without restating them.

// Strings
#include <string>
#include <string_view>

// Containers
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// IO
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>

// Concurrency
#include <atomic>
#include <mutex>
#include <thread>

// Utilities
#include <algorithm>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>

// C standard library
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#endif
