#pragma once

#include <string.h>
#include <assert.h>
#include "util.h"

/* 
CAPTAIN_LICKLY 和 CAPTAIN_UNLICKLY：这两个宏用于优化断言的性能。
在使用断言时，由于大多数情况下表达式是为真的，而断言失败是非常少见的，
因此使用预测分支优化可以提高程序性能。这里使用了 __builtin_expect 来提示编译器优化分支预测。
如果是 GCC 或 LLVM 编译器，就使用 __builtin_expect 来进行预测分支优化；否则就直接使用原始表达式。
 */
#if defined __GNUC__ || defined __llvm__
#   define CAPTAIN_LICKLY(x)       __builtin_expect(!!(x), 1)
#   define CAPTAIN_UNLICKLY(x)     __builtin_expect(!!(x), 0)
#else
#   define CAPTAIN_LICKLY(x)      (x)
#   define CAPTAIN_UNLICKLY(x)      (x)
#endif

/* 
CAPTAIN_ASSERT(x) 和 CAPTAIN_ASSERT2(x, w)：这两个宏分别用于检查表达式 x 是否为真。
如果表达式为假，则记录错误日志，输出断言失败的信息和回溯（backtrace），
并使用 assert(x) 终止程序执行。

回溯（backtrace）是一种在程序发生异常或错误时输出函数调用栈的技术。它可以帮助程序员快速定位问题所在的代码位置。
 */
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
