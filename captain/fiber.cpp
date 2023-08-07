#include "include/fiber.h"
#include "include/config.h"
#include "include/macro.h"
#include "include/log.h"
#include "include/scheduler.h"
#include <atomic>

namespace captain{

static Logger::ptr g_logger = CAPTAIN_LOG_NAME("system");

static std::atomic<uint64_t> s_fiber_id {0};
static std::atomic<uint64_t> s_fiber_count {0};

//线程的主协程  当前协程
static thread_local Fiber* t_fiber = nullptr;
//main/master/主 协程
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

//获取当前正在执行的协程的唯一标识符（ID）
uint64_t Fiber::GetFiberId() {
    if(t_fiber) {
        return t_fiber->getId();
    }
    return 0;
}

//默认构造函数其实就是这个线程的主协程，没有参数。
Fiber::Fiber() {
    //CAPTAIN_LOG_DEBUG(g_logger) << "====Fiber::Fiber()无参====";
    m_state = EXEC;
    SetThis(this);

    if(getcontext(&m_ctx)) {
        CAPTAIN_ASSERT2(false, "getcontext");
    }
    //总协程+1
    ++s_fiber_count;

    CAPTAIN_LOG_DEBUG(g_logger) << "Fiber::Fiber";
}

//有回调函数的这个真正开启了新的协程，需要分配栈空间
Fiber::Fiber(std::function<void()> cb, size_t stacksize, bool use_caller)
    :m_id(++s_fiber_id)
    ,m_cb(cb) {
    //CAPTAIN_LOG_DEBUG(g_logger) << "====Fiber::Fiber有参====" ;
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
    //通过断言确保协程的栈已经分配（m_stack 不为空），并且协程的状态为 INIT、TERM 或 EXCEPT，这些状态表明协程处于可重置的状态。
    CAPTAIN_ASSERT(m_stack);
    CAPTAIN_ASSERT(m_state == TERM
            || m_state == EXCEPT
            || m_state == INIT);
    m_cb = cb; //将传入的新执行函数 cb 设置为协程的回调函数 m_cb。
    if(getcontext(&m_ctx)) { //获取当前协程的上下文
        CAPTAIN_ASSERT2(false, "getcontext");
    }
    //设置协程的上下文（context）中的堆栈信息，将之前分配的堆栈
    //（m_stack）和堆栈大小（m_stacksize）赋给上下文的相应字段。
    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;
    //使用 makecontext 函数将协程的回调函数 Fiber::MainFunc 设置为协程的执行函数。
    //makecontext 它用于为协程上下文设置一个新的执行函数。
    makecontext(&m_ctx, &Fiber::MainFunc, 0);
    m_state = INIT; //表示协程已经重置为初始状态，可以再次执行。
}

// 强行把当前协程切换为目标执行协程
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

//切换到当前协程执行 将控制权从调用者切换到当前协程，并将协程的状态设置为 EXEC，表示该协程正在执行。
//swapIn 从【线程的主协程】切换到【当前协程】
void Fiber::swapIn() {
    SetThis(this);//将当前协程（this 指针所指向的协程对象）设置为线程局部存储中的当前协程。
    CAPTAIN_ASSERT(m_state != EXEC); //确保当前协程不处于执行状态，避免重复切换。
    m_state = EXEC; //将当前协程的状态设置为执行状态（EXEC）。
    //swapcontext 函数的作用是实现上下文之间的切换，将执行流程从主协程切换到当前协程，同时保存主协程的上下文，以便后续切换回主协程时恢复执行。
    if(swapcontext(&Scheduler::GetMainFiber()->m_ctx, &m_ctx)) {
    //if(swapcontext(&t_threadFiber->m_ctx, &m_ctx)) {
        CAPTAIN_ASSERT2(false, "swapcontext");
    }
}

//切换到后台执行
//swapOut 从【当前协程】切换到【线程的主协程】
void Fiber::swapOut() {
    SetThis(Scheduler::GetMainFiber());
    //当前协程的上下文（保存在 m_ctx）切换回主协程的上下文（保存在 Scheduler::GetMainFiber()->m_ctx）
    if(swapcontext(&m_ctx, &Scheduler::GetMainFiber()->m_ctx)) {
    //if(swapcontext(&m_ctx, &t_threadFiber->m_ctx)) {
        CAPTAIN_ASSERT2(false, "swapcontext");
    }
    //CAPTAIN_LOG_DEBUG(g_logger) << "====swapOut()====";
}

/* void Fiber::swapOut() {
    if(t_fiber != Scheduler::GetMainFiber()){
        SetThis(Scheduler::GetMainFiber());
        if(swapcontext(&m_ctx, &Scheduler::GetMainFiber()->m_ctx)) {
            CAPTAIN_ASSERT2(false, "swapcontext");
        }
    }else {
        SetThis(t_threadFiber.get());
        if(swapcontext(&m_ctx, &t_threadFiber->m_ctx)) {
            CAPTAIN_ASSERT2(false, "swapcontext");
        }
    }
} */


//设置当前协程
//静态成员函数，用于将当前协程设置为线程局部存储中的当前协程指针 t_fiber。
void Fiber::SetThis(Fiber* f) {
    t_fiber = f;
}

//返回当前协程，如果该线程没有协程，GetThis()会初始化一个主协程
Fiber::ptr Fiber::GetThis() {
    /* 
    如果 t_fiber 不为空，则说明当前有正在执行的协程，直接返回当前协程的 shared_ptr（共享智能指针），
    以确保当前协程对象的生命周期受到引用计数管理，避免在协程执行过程中被意外释放。
     */
    if(t_fiber) {
        return t_fiber->shared_from_this();
    }
    //如果当前没有正在执行的协程，则会创建一个主协程，并将其设置为当前协程
    Fiber::ptr main_fiber(new Fiber);
    //通过断言来检查主协程 t_fiber 是否指向了新创建的主协程对象。
    CAPTAIN_ASSERT(t_fiber == main_fiber.get());
    t_threadFiber = main_fiber; //同时将主协程的 shared_ptr 保存在 t_threadFiber 变量中，以备后续协程切换使用。
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

//协程的执行函数，该函数被设置为协程上下文的入口点。它负责执行协程函数，并在执行过程中处理可能出现的异常情况。
void Fiber::MainFunc() {
    //获取当前正在执行的协程指针。这里的 Fiber::ptr 是协程智能指针，它允许在协程执行结束后自动释放资源。
    Fiber::ptr cur = GetThis();
    CAPTAIN_ASSERT(cur); //确保已成功获取当前协程。
    try { //如果没有异常抛出，代码将会执行 try 块中的内容
        cur->m_cb(); //执行协程的主要逻辑，即协程函数。
        //CAPTAIN_LOG_DEBUG(g_logger) << "====cur->m_cb()====";
        cur->m_cb = nullptr;// 当协程函数执行完毕后，将其回调函数指针置空，确保在协程函数返回后不再执行该函数。
        cur->m_state = TERM; //协程已经正常终止
    } catch (std::exception& ex) {
        cur->m_state = EXCEPT;
        //将异常信息和协程ID记录到日志中，并输出协程的调用栈。
        CAPTAIN_LOG_ERROR(g_logger) << "Fiber Except: " << ex.what()
            << " fiber_id=" << cur->getId()
            << std::endl
            << captain::BacktraceToString();
    } catch (...) { //捕获其他类型的异常
        cur->m_state = EXCEPT;
        CAPTAIN_LOG_ERROR(g_logger) << "Fiber Except"
            << " fiber_id=" << cur->getId()
            << std::endl
            << captain::BacktraceToString();
    }

    auto raw_ptr = cur.get(); //获取当前协程指针的原始指针（非智能指针）
    cur.reset();//通过 reset() 方法，将当前协程智能指针置空。此时，如果没有其他智能指针引用当前协程对象，该对象将被释放。
    raw_ptr->swapOut(); //将协程切换到后台执行，释放当前协程的执行权。
    //这是一个断言，用于在调试时确保不会到达此处(因为在之前已经通过 swapOut() 将当前协程切换到后台)。如果执行到这里，表示发生了异常或错误。
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
