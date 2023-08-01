#pragma once

#include <string.h>
#include <assert.h>
#include "util.h"

#if defined __GNUC__ || defined __llvm__
#   define CAPTAIN_LICKLY(x)       __builtin_expect(!!(x), 1)
#   define CAPTAIN_UNLICKLY(x)     __builtin_expect(!!(x), 0)
#else
#   define CAPTAIN_LICKLY(x)      (x)
#   define CAPTAIN_UNLICKLY(x)      (x)
#endif

#define CAPTAIN_ASSERT(x) \
    if(CAPTAIN_UNLICKLY(!(x))) { \
        CAPTAIN_LOG_ERROR(CAPTAIN_LOG_ROOT()) << "ASSERTION: " #x \
            << "\nbacktrace:\n" \
            << captain::BacktraceToString(100, 2, "    "); \
        assert(x); \
    }

#define CAPTAIN_ASSERT2(x, w) \
    if(CAPTAIN_UNLICKLY(!(x))) { \
        CAPTAIN_LOG_ERROR(CAPTAIN_LOG_ROOT()) << "ASSERTION: " #x \
            << "\n" << w \
            << "\nbacktrace:\n" \
            << captain::BacktraceToString(100, 2, "    "); \
        assert(x); \
    }
