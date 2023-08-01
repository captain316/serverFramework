#include "../captain/include/captain.h"
#include <assert.h>

captain::Logger::ptr g_logger = CAPTAIN_LOG_ROOT();
/* 
在调用栈跟踪中，我们希望获取程序当前的执行路径，即函数调用的序列，以便在出现异常或错误时进行调试。这对于排查问题和定位 bug 非常有帮助。
backtrace 函数是一个用于获取调用栈信息的函数，它会将当前调用栈中的返回地址存储在一个指针数组中，并返回数组的大小。这些返回地址可以用来还原程序的执行路径。
backtrace 函数将当前调用栈中的返回地址存储在 array 指针数组中，并返回实际存储的返回地址个数。这样，通过遍历 array 数组，就可以获取调用栈中的返回地址，并将其转换成字符串表示，用于调试或日志记录。
 */
void test_assert() {
    CAPTAIN_LOG_INFO(g_logger) << captain::BacktraceToString(10);
    CAPTAIN_ASSERT(false);
    CAPTAIN_ASSERT2(0 == 1, "abcdef xx");
}

int main(int argc, char** argv) {
    //assert(0);
    test_assert();
    return 0;
}
