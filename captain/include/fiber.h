#pragma once

#include <memory>
#include <functional>
#include <ucontext.h>
#include "thread.h"

namespace captain {

class Scheduler;
/* 
这段代码定义了一个名为 Fiber 的类，该类继承自 std::enable_shared_from_this<Fiber>，
意味着它具有一些与 std::shared_ptr 相关的特性，主要用于管理共享指针的生命周期。
std::enable_shared_from_this 是 C++11 中提供的辅助类模板，用于允许对象在使用 
std::shared_ptr 进行共享拥有时，能够安全地获取指向自身的 std::shared_ptr，
以防止在拥有对象的智能指针析构时，出现悬空指针问题。

继承自 std::enable_shared_from_this<Fiber> 可以使 Fiber 类的对象通过调用 
shared_from_this() 函数获取指向自身的 std::shared_ptr。

这个特性在协程的管理和生命周期中非常有用，因为协程可能在其他地方共享和持有，
通过继承 std::enable_shared_from_this，可以安全地获取协程对象的共享指针而不会导致重复创建和销毁对象，避免了潜在的内存泄漏和非法访问问题。
 */
class Fiber : public std::enable_shared_from_this<Fiber> {
friend class Scheduler;
public:
    typedef std::shared_ptr<Fiber> ptr;

    enum State {
        INIT,
        HOLD,
        EXEC,
        TERM,
        READY,
        EXCEPT
    };
private:
    Fiber();

public:
    Fiber(std::function<void()> cb, size_t stacksize = 0, bool use_caller = false);
    ~Fiber();

    //重置协程函数，并重置状态
    //INIT，TERM
    void reset(std::function<void()> cb);
    //切换到当前协程执行
    void swapIn();
    //切换到后台执行
    void swapOut();

    void call();
    void back();

    uint64_t getId() const { return m_id;}
    State getState() const { return m_state;}
public:
    //设置当前协程
    static void SetThis(Fiber* f);
    //返回当前协程
    static Fiber::ptr GetThis();
    //协程切换到后台，并且设置为Ready状态
    static void YieldToReady();
    //协程切换到后台，并且设置为Hold状态
    static void YieldToHold();
    //总协程数
    static uint64_t TotalFibers();
    //协程的主函数。
    static void MainFunc();
    //调用者的主函数。
    static void CallerMainFunc();
    //获取当前协程的唯一标识符。
    static uint64_t GetFiberId();
private:
    uint64_t m_id = 0;
    uint32_t m_stacksize = 0;
    State m_state = INIT;

    ucontext_t m_ctx;
    void* m_stack = nullptr;
    //协程的回调函数，即协程的执行体。
    std::function<void()> m_cb;
};

}

