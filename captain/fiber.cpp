#include "fiber.h"
#include "config.h"
#include "macro.h"
#include "log.h"
#include "include/scheduler.h"
#include <atomic>

namespace captain{

static Logger::ptr g_logger = CAPTAIN_LOG_NAME("system");

static std::atomic<uint64_t> s_fiber_id {0};
static std::atomic<uint64_t> s_fiber_count {0};

//线程的主协程  当前协程
static thread_local Fiber* t_fiber = nullptr;
//main/master协程？ 
static thread_local Fiber::ptr t_threadFiber = nullptr;
//协程大小  默认1M
static ConfigVar<uint32_t>::ptr g_fiber_stack_size =
    Config::Lookup<uint32_t>("fiber.stack_size", 1024 * 1024, "fiber stack size");

//测试内存分配的效率
class MallocStackAllocator {
public:
    static void* Alloc(size_t size) {
        return malloc(size);
    }

    static void Dealloc(void* vp, size_t size) {
        return free(vp);
    }
};
//赋值
using StackAllocator = MallocStackAllocator;

uint64_t Fiber::GetFiberId() {
    if(t_fiber) {
        return t_fiber->getId();
    }
    return 0;
}

Fiber::Fiber() {
    m_state = EXEC;
    SetThis(this);

    if(getcontext(&m_ctx)) {
        CAPTAIN_ASSERT2(false, "getcontext");
    }
    //总协程+1
    ++s_fiber_count;

    CAPTAIN_LOG_DEBUG(g_logger) << "Fiber::Fiber";
}

Fiber::Fiber(std::function<void()> cb, size_t stacksize, bool use_caller)
    :m_id(++s_fiber_id)
    ,m_cb(cb) {
    ++s_fiber_count;
    //m_stacksize 初始化为传入的 stacksize，如果 stacksize 为0，则使用全局配置 g_fiber_stack_size 的值作为栈大小。
    m_stacksize = stacksize ? stacksize : g_fiber_stack_size->getValue();
    //栈生成 使用 StackAllocator 类分配 m_stacksize 大小的栈内存
    m_stack = StackAllocator::Alloc(m_stacksize);
    //调用 getcontext 函数获取当前上下文，并将其保存在 m_ctx 成员变量中。
    if(getcontext(&m_ctx)) {
        CAPTAIN_ASSERT2(false, "getcontext");
    }
/*
上下文包含了当前线程（或协程）的寄存器状态，栈信息等，可以理解为保存了当前线程执行的上下文状态，
包括当前函数调用的返回地址和栈指针等。通过 getcontext 获取当前协程的上下文信息后，
可以在之后需要切换到该协程执行时，通过 setcontext 或 swapcontext 将保存的上下文恢复，
从而实现协程的切换和调度。在协程的切换和恢复过程中，保存和恢复的上下文信息是关键，
它决定了协程从何处继续执行和在何处暂停。 
 */

    //设置协程上下文 m_ctx 的 uc_link 指向为 nullptr，表示当协程执行完毕时不返回到任何上下文
    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;

    //使用 makecontext 函数为协程上下文设置执行函数。如果 use_caller 为 false，
    //则设置执行函数为 Fiber::MainFunc，否则设置为 Fiber::CallerMainFunc。
    if(!use_caller) {
        makecontext(&m_ctx, &Fiber::MainFunc, 0);
    } else {
        makecontext(&m_ctx, &Fiber::CallerMainFunc, 0);
    }

    CAPTAIN_LOG_DEBUG(g_logger) << "Fiber::Fiber id=" << m_id;
}

//Fiber::~Fiber() 用于释放协程对象的资源，包括栈内存的回收，并在必要时进行协程切换，确保不会析构当前正在执行的协程对象。
Fiber::~Fiber() {
    --s_fiber_count;
    if(m_stack) { //如果协程对象有分配栈内存（m_stack 不为 nullptr）
        //协程必须在终止状态或异常状态下才能被析构。
        CAPTAIN_ASSERT(m_state == TERM
                || m_state == EXCEPT
                || m_state == INIT);
        //回收栈 回收之前分配的协程栈内存。
        StackAllocator::Dealloc(m_stack, m_stacksize);
    } else { //如果协程对象没有分配栈内存（m_stack 为 nullptr）
        CAPTAIN_ASSERT(!m_cb);//断言当前协程没有关联的回调函数。因为没有分配栈内存，这意味着这个协程没有执行过任务。
        CAPTAIN_ASSERT(m_state == EXEC);//断言当前协程状态为执行状态（EXEC），这表示当前协程正在运行中。

        Fiber* cur = t_fiber; //获取当前正在运行的协程对象。
        /* 
        如果当前运行的协程对象指针与即将析构的协程对象指针相同，即当前正在析构的协程是当前正在执行的协程，
        则将当前正在执行的协程指针设置为 nullptr，表示当前协程将不再继续执行。
         */
        if(cur == this) {
            SetThis(nullptr);
        }
    }
    CAPTAIN_LOG_DEBUG(g_logger) << "Fiber::~Fiber id=" << m_id;
}

//重置协程函数，并重置状态
//INIT，TERM
void Fiber::reset(std::function<void()> cb) {
    CAPTAIN_ASSERT(m_stack);
    CAPTAIN_ASSERT(m_state == TERM
            || m_state == EXCEPT
            || m_state == INIT);
    m_cb = cb;
    if(getcontext(&m_ctx)) {
        CAPTAIN_ASSERT2(false, "getcontext");
    }

    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;

    makecontext(&m_ctx, &Fiber::MainFunc, 0);
    m_state = INIT;
}

void Fiber::call() {
    SetThis(this);
    m_state = EXEC;
    CAPTAIN_LOG_ERROR(g_logger) << getId();
    if(swapcontext(&t_threadFiber->m_ctx, &m_ctx)) {
        CAPTAIN_ASSERT2(false, "swapcontext");
    }
}

void Fiber::back() {
    SetThis(t_threadFiber.get());
    if(swapcontext(&m_ctx, &t_threadFiber->m_ctx)) {
        CAPTAIN_ASSERT2(false, "swapcontext");
    }
}

//切换到当前协程执行
void Fiber::swapIn() {
    SetThis(this);
    CAPTAIN_ASSERT(m_state != EXEC);
    m_state = EXEC;
    if(swapcontext(&Scheduler::GetMainFiber()->m_ctx, &m_ctx)) {
        CAPTAIN_ASSERT2(false, "swapcontext");
    }
}

//切换到后台执行
void Fiber::swapOut() {
    SetThis(Scheduler::GetMainFiber());
    if(swapcontext(&m_ctx, &Scheduler::GetMainFiber()->m_ctx)) {
        CAPTAIN_ASSERT2(false, "swapcontext");
    }
}

//设置当前协程
void Fiber::SetThis(Fiber* f) {
    t_fiber = f;
}

//返回当前协程
Fiber::ptr Fiber::GetThis() {
    if(t_fiber) {
        return t_fiber->shared_from_this();
    }
    Fiber::ptr main_fiber(new Fiber);
    CAPTAIN_ASSERT(t_fiber == main_fiber.get());
    t_threadFiber = main_fiber;
    return t_fiber->shared_from_this();
}

//协程切换到后台，并且设置为Ready状态
void Fiber::YieldToReady() {
    Fiber::ptr cur = GetThis();
    cur->m_state = READY;
    cur->swapOut();
}

//协程切换到后台，并且设置为Hold状态
void Fiber::YieldToHold() {
    Fiber::ptr cur = GetThis();
    cur->m_state = HOLD;
    cur->swapOut();
}

//总协程数
uint64_t Fiber::TotalFibers() {
    return s_fiber_count;
}

void Fiber::MainFunc() {
    Fiber::ptr cur = GetThis();
    CAPTAIN_ASSERT(cur);
    try {
        cur->m_cb();
        cur->m_cb = nullptr;
        cur->m_state = TERM;
    } catch (std::exception& ex) {
        cur->m_state = EXCEPT;
        CAPTAIN_LOG_ERROR(g_logger) << "Fiber Except: " << ex.what()
            << " fiber_id=" << cur->getId()
            << std::endl
            << captain::BacktraceToString();
    } catch (...) {
        cur->m_state = EXCEPT;
        CAPTAIN_LOG_ERROR(g_logger) << "Fiber Except"
            << " fiber_id=" << cur->getId()
            << std::endl
            << captain::BacktraceToString();
    }

    auto raw_ptr = cur.get();
    cur.reset();
    raw_ptr->swapOut();

    CAPTAIN_ASSERT2(false, "never reach fiber_id=" + std::to_string(raw_ptr->getId()));
}

void Fiber::CallerMainFunc() {
    Fiber::ptr cur = GetThis();
    CAPTAIN_ASSERT(cur);
    try {
        cur->m_cb();
        cur->m_cb = nullptr;
        cur->m_state = TERM;
    } catch (std::exception& ex) {
        cur->m_state = EXCEPT;
        CAPTAIN_LOG_ERROR(g_logger) << "Fiber Except: " << ex.what()
            << " fiber_id=" << cur->getId()
            << std::endl
            << captain::BacktraceToString();
    } catch (...) {
        cur->m_state = EXCEPT;
        CAPTAIN_LOG_ERROR(g_logger) << "Fiber Except"
            << " fiber_id=" << cur->getId()
            << std::endl
            << captain::BacktraceToString();
    }

    auto raw_ptr = cur.get();
    cur.reset();
    raw_ptr->back();
    CAPTAIN_ASSERT2(false, "never reach fiber_id=" + std::to_string(raw_ptr->getId()));

}

}
