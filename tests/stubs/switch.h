#pragma once
#include <cstdint>
#include <cstring>
using u64 = uint64_t;
#define R_FAILED(x) ((x) != 0)
inline int setGetSystemLanguage(u64* code) {
    *code = 0;
    std::memcpy(code, "en-US", 5);
    return 0;
}
