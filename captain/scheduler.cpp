#include "include/scheduler.h"
#include "include/log.h"
#include "include/macro.h"
//#include "hook.h"

namespace captain {

static captain::Logger::ptr g_logger = CAPTAIN_LOG_NAME("system");
//当前线程所属的调度器对象的指针
static thread_local Scheduler* t_scheduler = nullptr;
//当前协程的主协程函数
static thread_local Fiber* t_fiber = nullptr;

Scheduler::Scheduler(size_t threads, bool use_caller, const std::string& name)
    :m_name(name) {
    CAPTAIN_ASSERT(threads > 0);

    if(use_caller) { //当前线程将执行协程调度器的任务。
        //如果该线程没有协程，GetThis()会初始化一个主协程
        captain::Fiber::GetThis();//创建当前线程的根协程（root fiber）并将其与当前线程绑定，以便当前线程能够参与协程调度。
        --threads; //因为当前线程已经作为调度器的一个工作线程。
        
        //确保在当前线程之前没有设置调度器对象。因为一个线程只能有一个调度器对象。
        CAPTAIN_ASSERT(GetThis() == nullptr);
        //将当前调度器对象的指针设置为静态线程局部变量 t_scheduler 的值
        t_scheduler = this;
        //创建了根/主协程对象 m_rootFiber，使用 std::bind 绑定了 Scheduler::run 方法作为协程的执行函数。根协程会在执行 Scheduler::run 方法时启动协程调度器的运行。
        m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true));
        captain::Thread::SetName(m_name); //为当前线程设置线程名称
        //将根协程指针设置为静态 线程局部变量 t_fiber 的值。因为当前线程将作为调度器的一个工作线程，所以当前线程的 t_fiber 就是根协程 m_rootFiber。
        t_fiber = m_rootFiber.get();
        //获取当前线程的线程 ID
        m_rootThread = captain::GetThreadId();
        m_threadIds.push_back(m_rootThread);
    } else { //当前线程不会作为调度器的一个工作线程，它将作为普通的用户线程。
        m_rootThread = -1;
    }
    m_threadCount = threads;
}

Scheduler::~Scheduler() {
    //在析构函数中，应该确保调度器处于停止状态，以避免在调度器还在运行时被析构。
    CAPTAIN_ASSERT(m_stopping);
    if(GetThis() == this) { //当前调度器对象是否是当前线程的调度器对象
        t_scheduler = nullptr;
    }
}

Scheduler* Scheduler::GetThis() {
    return t_scheduler;
}

Fiber* Scheduler::GetMainFiber() {
    return t_fiber;
}
//启动调度器
void Scheduler::start() {
    MutexType::Lock lock(m_mutex);
    //检查调度器的状态 m_stopping 是否为 false，如果不是，说明调度器已经在运行，无需再次启动，直接返回。
    if(!m_stopping) {
        return;
    }
    m_stopping = false;//调度器正在运行中。
    CAPTAIN_ASSERT(m_threads.empty());
    //创建线程池 m_threads：根据之前设置的线程数量 m_threadCount，创建对应数量的线程，并将其存储在 m_threads 容器中。
    m_threads.resize(m_threadCount);
    //将线程 ID 添加到 m_threadIds
    for(size_t i = 0; i < m_threadCount; ++i) {
        m_threads[i].reset(new Thread(std::bind(&Scheduler::run, this)
                            , m_name + "_" + std::to_string(i)));
        m_threadIds.push_back(m_threads[i]->getId());
    }
    lock.unlock();

    // if(m_rootFiber) {
    //     //m_rootFiber->swapIn();
    //     m_rootFiber->call();
    //     CAPTAIN_LOG_INFO(g_logger) << "call out " << m_rootFiber->getState();
    //  }
}
//停止调度器的运行
void Scheduler::stop() {
    m_autoStop = true; //调度器需要自动停止。
    //检查根协程状态和线程池状态：如果存在根协程 m_rootFiber 并且线程池的数量为0，并且根协程的状态为 Fiber::TERM 或 Fiber::INIT，则认为调度器已经停止，打印日志，并直接返回。
    if(m_rootFiber
            && m_threadCount == 0
            && (m_rootFiber->getState() == Fiber::TERM
                || m_rootFiber->getState() == Fiber::INIT)) {
        CAPTAIN_LOG_INFO(g_logger) << this << " stopped";
        m_stopping = true;

        if(stopping()) {
            return;
        }
    }

    //bool exit_on_this_fiber = false;
/* 
检查当前线程是否是根线程：通过 m_rootThread 的值来判断当前线程是否是调度器的根线程。
如果是根线程，则要求 GetThis() == this，即当前线程必须关联的是当前调度器。
如果不是根线程，则要求 GetThis() != this，即当前线程不应该关联任何调度器。
这样做是为了确保在停止调度器时，只能由根线程执行，其他线程不能执行停止操作。
 */
    if(m_rootThread != -1) {
        //说明是use_caller线程
        CAPTAIN_ASSERT(GetThis() == this);
    } else {
        CAPTAIN_ASSERT(GetThis() != this);
    }

    m_stopping = true; //度器正在停止中
    //使用 tickle() 唤醒所有线程
    for(size_t i = 0; i < m_threadCount; ++i) {
        tickle();
    }
    //使用 tickle() 唤醒根协程
    if(m_rootFiber) {
        tickle();
    }
    //执行根协程
    if(m_rootFiber) {
        //while(!stopping()) {
        //    if(m_rootFiber->getState() == Fiber::TERM
        //            || m_rootFiber->getState() == Fiber::EXCEPT) {
        //        m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true));
        //        CAPTAIN_LOG_INFO(g_logger) << " root fiber is term, reset";
        //        t_fiber = m_rootFiber.get();
        //    }
        //    m_rootFiber->call();
        //}
        if(!stopping()) {
            m_rootFiber->call();
        }
    }
/* 
等待所有线程结束：获取互斥锁 m_mutex 并交换线程池容器 m_threads 的内容到临时变量 thrs，
然后遍历临时变量 thrs，调用每个线程的 join() 方法，等待所有线程结束。
 */
    std::vector<Thread::ptr> thrs;
    {
        MutexType::Lock lock(m_mutex);
        thrs.swap(m_threads);
    }

    for(auto& i : thrs) {
        i->join();
    }
    //if(exit_on_this_fiber) {
    //}
}

void Scheduler::setThis() {
    t_scheduler = this;
}

/* 
1、设置当前线程的scheduler
2、设置当前线程的run,fiber
3、协程调度循环while(true)
    1）协程消息队列里是否有任务
    2）无任务执行，执行idle
 */
void Scheduler::run() {
    CAPTAIN_LOG_INFO(g_logger) << "run";
    //set_hook_enable(true);
    setThis();
    //return;
    //检查当前线程的 ID 是否等于调度器的根线程 ID
    if(captain::GetThreadId() != m_rootThread) {
        t_fiber = Fiber::GetThis().get();
    }
    //创建一个空闲的协程，该协程的任务为调用 Scheduler::idle() 方法。
    Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
    Fiber::ptr cb_fiber; //回调函数  function函数的协程

    FiberAndThread ft;
    while(true) {
        ft.reset();
        bool tickle_me = false;
        bool is_active = false;
        {
            MutexType::Lock lock(m_mutex);
            auto it = m_fibers.begin();
            while(it != m_fibers.end()) {
                if(it->thread != -1 && it->thread != captain::GetThreadId()) {
                    ++it;
                    tickle_me = true;
                    continue;
                }

                CAPTAIN_ASSERT(it->fiber || it->cb);
                if(it->fiber && it->fiber->getState() == Fiber::EXEC) {
                    ++it;
                    continue;
                }

                ft = *it;
                m_fibers.erase(it);
                ++m_activeThreadCount;
                is_active = true;
                break;
            }
        }

        if(tickle_me) {
            tickle();
        }

        if(ft.fiber && (ft.fiber->getState() != Fiber::TERM
                        && ft.fiber->getState() != Fiber::EXCEPT)) {
            ft.fiber->swapIn();
            --m_activeThreadCount;

            if(ft.fiber->getState() == Fiber::READY) {
                schedule(ft.fiber);
            } else if(ft.fiber->getState() != Fiber::TERM
                    && ft.fiber->getState() != Fiber::EXCEPT) {
                ft.fiber->m_state = Fiber::HOLD;
            }
            ft.reset();
        } else if(ft.cb) {
            if(cb_fiber) {
                cb_fiber->reset(ft.cb);
            } else {
                cb_fiber.reset(new Fiber(ft.cb));
            }
            ft.reset();
            cb_fiber->swapIn();
            --m_activeThreadCount;
            if(cb_fiber->getState() == Fiber::READY) {
                schedule(cb_fiber);
                cb_fiber.reset();
            } else if(cb_fiber->getState() == Fiber::EXCEPT
                    || cb_fiber->getState() == Fiber::TERM) {
                cb_fiber->reset(nullptr);
            } else {//if(cb_fiber->getState() != Fiber::TERM) {
                cb_fiber->m_state = Fiber::HOLD;
                cb_fiber.reset();
            }
        } else {
            if(is_active) {
                --m_activeThreadCount;
                continue;
            }
            if(idle_fiber->getState() == Fiber::TERM) {
                CAPTAIN_LOG_INFO(g_logger) << "idle fiber term";
                break;
            }

            ++m_idleThreadCount;
            idle_fiber->swapIn();
            --m_idleThreadCount;
            if(idle_fiber->getState() != Fiber::TERM
                    && idle_fiber->getState() != Fiber::EXCEPT) {
                idle_fiber->m_state = Fiber::HOLD;
            }
        }
    }
}

void Scheduler::tickle() {
    CAPTAIN_LOG_INFO(g_logger) << "tickle";
}

bool Scheduler::stopping() {
    MutexType::Lock lock(m_mutex);
    return m_autoStop && m_stopping
        && m_fibers.empty() && m_activeThreadCount == 0;
}

void Scheduler::idle() {
    CAPTAIN_LOG_INFO(g_logger) << "idle";
    while(!stopping()) {
        captain::Fiber::YieldToHold();
    }
}

}