#pragma once

#include <memory>
#include <vector>
#include <list>
#include "fiber.h"
#include "thread.h"

namespace captain {
/* Scheduler
1、线程池，分配一组线程
2、协程调度器，将协程制定到相应的线程上去执行。
 */
class Scheduler {
public:
    typedef std::shared_ptr<Scheduler> ptr;
    typedef Mutex MutexType;
    //use_caller：在某个线程执行了协程调度器的构造函数的时候，如果设置use_caller = true,意味着该线程也会纳入协程调度器中
    Scheduler(size_t threads = 1, bool use_caller = true, const std::string& name = "");
    virtual ~Scheduler();

    const std::string& getName() const { return m_name;}

    static Scheduler* GetThis(); //获取当前线程的调度器对象。
    static Fiber* GetMainFiber();//获取调度器主协程（MainFiber）

    void start();
    void stop();
    //用于将任务（协程或回调函数）（单个）添加到调度器的任务队列，并触发调度器唤醒（tickle）以执行任务。
    template<class FiberOrCb>
    void schedule(FiberOrCb fc, int thread = -1) {
        bool need_tickle = false;
        {
            MutexType::Lock lock(m_mutex);
            need_tickle = scheduleNoLock(fc, thread);
        }

        if(need_tickle) {
            tickle();
        }
    }
    //用于将一系列任务（协程或回调函数）添加到调度器的任务队列，并触发调度器唤醒（tickle）以执行任务。
    template<class InputIterator>
    void schedule(InputIterator begin, InputIterator end) {
        bool need_tickle = false;
        {
            MutexType::Lock lock(m_mutex);
            while(begin != end) {
                need_tickle = scheduleNoLock(&*begin, -1) || need_tickle;
                ++begin;
            }
        }
        if(need_tickle) {
            tickle();
        }
    }
protected:
    virtual void tickle();
    void run();
    virtual bool stopping();
    virtual void idle();

    void setThis();

    bool hasIdleThreads() { return m_idleThreadCount > 0;}
private:
    //用于将要调度的任务（协程或回调函数）添加到调度器的任务队列（m_fibers 链表）中，并返回一个布尔值表示是否需要唤醒（tickle）调度器。
    template<class FiberOrCb>
    bool scheduleNoLock(FiberOrCb fc, int thread) {
        bool need_tickle = m_fibers.empty();
        FiberAndThread ft(fc, thread);
        if(ft.fiber || ft.cb) {
            m_fibers.push_back(ft);
        }
        //如果返回值为 true，表示任务队列之前为空，现在有任务可供调度，因此调用者可能需要唤醒调度器以执行任务。
        return need_tickle;
    }
private:
    struct FiberAndThread {
        Fiber::ptr fiber;
        std::function<void()> cb;
        int thread;  //线程id
        //传入智能指针的对象  在栈上
        FiberAndThread(Fiber::ptr f, int thr)
            :fiber(f), thread(thr) {
        }
        //传入智能指针对象的指针（在堆上），swap掉智能指针，防止引用释放问题
        FiberAndThread(Fiber::ptr* f, int thr)
            :thread(thr) {
            fiber.swap(*f);
        }

        FiberAndThread(std::function<void()> f, int thr)
            :cb(f), thread(thr) {
        }

        FiberAndThread(std::function<void()>* f, int thr)
            :thread(thr) {
            cb.swap(*f);
        }

        FiberAndThread()
            :thread(-1) {
        }

        void reset() {
            fiber = nullptr;
            cb = nullptr;
            thread = -1;
        }
    };
private:
    MutexType m_mutex;
    std::vector<Thread::ptr> m_threads; //线程池
    std::list<FiberAndThread> m_fibers; //存储要调度的任务（协程或回调函数）以及指定它们执行的线程。
    Fiber::ptr m_rootFiber;
    std::string m_name;
protected:
    std::vector<int> m_threadIds;
    size_t m_threadCount = 0;
    std::atomic<size_t> m_activeThreadCount = {0};
    std::atomic<size_t> m_idleThreadCount = {0};
    bool m_stopping = true;
    bool m_autoStop = false;
    int m_rootThread = 0;
};

}
