#include "include/util.h"
#include <execinfo.h>
#include <sys/time.h>

#include "include/log.h"
#include "include/fiber.h"

namespace captain {

static captain::Logger::ptr g_logger = CAPTAIN_LOG_NAME("system");

pid_t GetThreadId() {
    return syscall(SYS_gettid);
}

uint32_t GetFiberId() {
    return captain::Fiber::GetFiberId();
}
//获取当前线程的函数调用栈信息，并将栈帧信息存储在一个 std::vector<std::string> 类型的变量 bt 中
void Backtrace(std::vector<std::string>& bt, int size, int skip) {
    // 分配一块内存来存储调用栈的返回地址
    // backtrace 函数需要一个 void** 类型的指针来存储返回地址，因此需要在堆上动态分配内存。
    void** array = (void**)malloc((sizeof(void*) * size));
    // 获取当前线程的调用栈信息，返回地址存储在 array 数组中，最多获取 size 个返回地址
    size_t s = ::backtrace(array, size);
    // 将返回地址转换为对应的函数名和偏移量，并存储在 strings 数组中
    char** strings = backtrace_symbols(array, s);
    if(strings == NULL) {
        CAPTAIN_LOG_ERROR(g_logger) << "backtrace_synbols error";
        return;
    }
    // 将栈帧信息转换为字符串，并加入到 bt 变量中
    for(size_t i = skip; i < s; ++i) {
        bt.push_back(strings[i]);
    }

    free(strings);
    free(array);
}

/* 
在调用栈跟踪中，我们希望获取程序当前的执行路径，即函数调用的序列，以便在出现异常或错误时进行调试。这对于排查问题和定位 bug 非常有帮助。
backtrace 函数是一个用于获取调用栈信息的函数，它会将当前调用栈中的返回地址存储在一个指针数组中，并返回数组的大小。这些返回地址可以用来还原程序的执行路径。
backtrace 函数将当前调用栈中的返回地址存储在 array 指针数组中，并返回实际存储的返回地址个数。这样，通过遍历 array 数组，就可以获取调用栈中的返回地址，并将其转换成字符串表示，用于调试或日志记录。
 */
//将调用栈跟踪信息转换成字符串形式并返回。
std::string BacktraceToString(int size, int skip, const std::string& prefix) {
    std::vector<std::string> bt;
    Backtrace(bt, size, skip);
    std::stringstream ss;
    for(size_t i = 0; i < bt.size(); ++i) {
        //prefix：const std::string& 类型的引用，用于指定每一行输出字符串的前缀。
        //在每次循环迭代中，将 prefix、bt[i]（即调用栈信息的一行）和换行符 std::endl 依次添加到 std::stringstream 对象 ss 中。
        ss << prefix << bt[i] << std::endl;
    }
    //ss 中会包含所有调用栈信息，并且每一行都带有指定的 prefix。
    return ss.str();
}

uint64_t GetCurrentMS() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000ul  + tv.tv_usec / 1000;
}

uint64_t GetCurrentUS() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 * 1000ul  + tv.tv_usec;
}

}