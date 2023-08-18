#pragma once
#include <memory>
#include <vector>
#include <set>
#include "thread.h"

namespace captain {

class TimerManager;
class Timer : public std::enable_shared_from_this<Timer> {
friend class TimerManager;
public:
    typedef std::shared_ptr<Timer> ptr;
    bool cancel();
    //重新设置执行时间
    bool refresh();
    //重置时间间隔
    bool reset(uint64_t ms, bool from_now);
private:
    Timer(uint64_t ms, std::function<void()> cb,
          bool recurring, TimerManager* manager);
    Timer(uint64_t next);
private:
    bool m_recurring = false;       //是否循环定时器
    uint64_t m_ms = 0;              //执行周期
    uint64_t m_next = 0;            //精确的执行时间
    std::function<void()> m_cb;     //定时器需要执行的任务
    TimerManager* m_manager = nullptr; //timer属于哪个manager
private:
    struct Comparator {
        bool operator()(const Timer::ptr& lhs, const Timer::ptr& rhs) const;
    };
};

class TimerManager {
friend class Timer;
public:
    typedef RWMutex RWMutexType;

    TimerManager();
    virtual ~TimerManager();
    //添加一个定时器
    Timer::ptr addTimer(uint64_t ms, std::function<void()> cb
                        ,bool recurring = false);
    //添加一个条件定时器  weak_ptr作条件，有个引用计数，引用计数=0，就没必要执行了
    Timer::ptr addConditionTimer(uint64_t ms, std::function<void()> cb
                        ,std::weak_ptr<void> weak_cond
                        ,bool recurring = false);
    //获取下一个定时器的执行时间
    uint64_t getNextTimer();
    //返回需要执行的回调函数（已经超过了等待时间的那些函数）在iomanager放到scheduler中执行
    void listExpiredCb(std::vector<std::function<void()> >& cbs);
    bool hasTimer();
protected:
    /* 当插入最前面的位置（插入的定时器是最小的）
       通知继承类（iomanager）已经有一个新的最小定时器，之前epoll_wait的那个时间有点大了，
       需要马上唤醒回来，重新设置一个时间
    */
    virtual void onTimerInsertedAtFront() = 0;
    //封装addTimer
    void addTimer(Timer::ptr val, RWMutexType::WriteLock& lock);
private:
    //检测系统时间
    bool detectClockRollover(uint64_t now_ms);
private:
    RWMutexType m_mutex;
    //定时器是有序的 Comparator 比较器
    //其实set容器有默认排序，但默认地址比较，但对我们而言没意义 
    std::set<Timer::ptr, Timer::Comparator> m_timers;
    bool m_tickled = false;
    //上一个执行的时间
    uint64_t m_previouseTime = 0;
};

}


