#include "include/timer.h"
#include "include/util.h"

namespace captain {

//比较函数  比较两个智能指针
bool Timer::Comparator::operator()(const Timer::ptr& lhs
                        ,const Timer::ptr& rhs) const {
    if(!lhs && !rhs) {  //两个都是空的
        return false;
    }
    if(!lhs) {
        return true;
    }
    if(!rhs) {
        return false;
    }
    //lhs执行时间 < rhs执行时间
    if(lhs->m_next < rhs->m_next) {
        return true;
    }
    if(rhs->m_next < lhs->m_next) {
        return false;
    }
    //时间一样，比较地址
    return lhs.get() < rhs.get();
}


Timer::Timer(uint64_t ms, std::function<void()> cb,
             bool recurring, TimerManager* manager)
    :m_recurring(recurring)
    ,m_ms(ms)
    ,m_cb(cb)
    ,m_manager(manager) {
    m_next = captain::GetCurrentMS() + m_ms; //m_ms执行间隔
}

Timer::Timer(uint64_t next)
    :m_next(next) {
}

bool Timer::cancel() {
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    if(m_cb) {
        m_cb = nullptr;
        //先找到定时器
        auto it = m_manager->m_timers.find(shared_from_this());
        m_manager->m_timers.erase(it);
        return true;
    }
    return false;
}

bool Timer::refresh() {
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    if(!m_cb) {
        return false;
    }
    auto it = m_manager->m_timers.find(shared_from_this());
    if(it == m_manager->m_timers.end()) {
        return false;
    }
    //先移除，后重置，再放回（set会重排序）  因为operator<是基于时间next做的，要先重置时间会影响比较位置
    m_manager->m_timers.erase(it);
    m_next = captain::GetCurrentMS() + m_ms;
    m_manager->m_timers.insert(shared_from_this());
    return true;
}

bool Timer::reset(uint64_t ms, bool from_now) {
    if(ms == m_ms && !from_now) {
        return true;
    }
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    if(!m_cb) {
        return false;
    }
    auto it = m_manager->m_timers.find(shared_from_this());
    if(it == m_manager->m_timers.end()) {
        return false;
    }
    m_manager->m_timers.erase(it);
    uint64_t start = 0;
    if(from_now) {
        start = captain::GetCurrentMS();
    } else {
        start = m_next - m_ms;
    }
    m_ms = ms;
    m_next = start + m_ms;
    m_manager->addTimer(shared_from_this(), lock);
    return true;

}

TimerManager::TimerManager() {
    m_previouseTime = captain::GetCurrentMS();
}

TimerManager::~TimerManager() {
}

//添加一个定时器
Timer::ptr TimerManager::addTimer(uint64_t ms, std::function<void()> cb
                                  ,bool recurring) {
    //创建一个timer
    Timer::ptr timer(new Timer(ms, cb, recurring, this));
    //把数据放到set里，需要一个锁
    RWMutexType::WriteLock lock(m_mutex);
    addTimer(timer, lock);
    return timer;
}

//条件定时器的辅助函数
static void OnTimer(std::weak_ptr<void> weak_cond, std::function<void()> cb) {
    //返回weak_ptr的智能指针，如果智能指针没有被释放就可以拿到，如果被释放了就是空值
    std::shared_ptr<void> tmp = weak_cond.lock();
    if(tmp) {
        cb();
    }
}

//条件定时器
Timer::ptr TimerManager::addConditionTimer(uint64_t ms, std::function<void()> cb
                                    ,std::weak_ptr<void> weak_cond
                                    ,bool recurring) {
    return addTimer(ms, std::bind(&OnTimer, weak_cond, cb), recurring);
}

uint64_t TimerManager::getNextTimer() {
    RWMutexType::ReadLock lock(m_mutex);
    m_tickled = false;
    if(m_timers.empty()) { //无任务执行
        return ~0ull;
    }
    //拿到首个定时器
    const Timer::ptr& next = *m_timers.begin();
    //获取当前时间
    uint64_t now_ms = captain::GetCurrentMS();
    if(now_ms >= next->m_next) {
        return 0;  //立刻执行
    } else {
        return next->m_next - now_ms;  //还需等待的时间
    }
}

void TimerManager::listExpiredCb(std::vector<std::function<void()> >& cbs) {
    uint64_t now_ms = captain::GetCurrentMS();
    //存放已经超时的数组
    std::vector<Timer::ptr> expired;
    {
        RWMutexType::ReadLock lock(m_mutex);
        if(m_timers.empty()) { //没有任何定时器需要执行
            return;
        }
    }
    RWMutexType::WriteLock lock(m_mutex);

    bool rollover = detectClockRollover(now_ms);
    if(!rollover && ((*m_timers.begin())->m_next > now_ms)) {
        return;
    }

    Timer::ptr now_timer(new Timer(now_ms));
    auto it = rollover ? m_timers.end() : m_timers.lower_bound(now_timer);
    while(it != m_timers.end() && (*it)->m_next == now_ms) {
        ++it;
    }
    //放进超时数组
    expired.insert(expired.begin(), m_timers.begin(), it);
    m_timers.erase(m_timers.begin(), it);
    cbs.reserve(expired.size());

    for(auto& timer : expired) {
        cbs.push_back(timer->m_cb);
        if(timer->m_recurring) {
            timer->m_next = now_ms + timer->m_ms;
            m_timers.insert(timer);
        } else {
            //防止回调函数用到智能指针，引用计数不会-1的情况发生
            timer->m_cb = nullptr;
        }
    }
}
//m_timers是个set容器，插入后排在第一个的是最小的定时器，也是即将执行的定时器，这时候需要唤醒原来的定时器
//通过onTimerInsertedAtFront唤醒epoll_wait 需要重新设置一个定时时间。
void TimerManager::addTimer(Timer::ptr val, RWMutexType::WriteLock& lock) {
    auto it = m_timers.insert(val).first; //返回一个pair，一个指明是否成功，一个指明位置 这里看位置
    bool at_front = (it == m_timers.begin()) && !m_tickled; //是否插入最前面的位置（插入的定时器是最小的），
    if(at_front) {                                          //true：时间是最小的，设计一个变量通知继承类（iomanager）已经有一个新的最小定时器，
        m_tickled = true;                                   //之前epoll_wait的那个时间有点大了，需要马上唤醒回来，重新设置一个时间
    }
    lock.unlock();

    if(at_front) {
        onTimerInsertedAtFront();
    }
}

bool TimerManager::detectClockRollover(uint64_t now_ms) {
    bool rollover = false;
    if(now_ms < m_previouseTime &&
            now_ms < (m_previouseTime - 60 * 60 * 1000)) {
        rollover = true;
    }
    m_previouseTime = now_ms;
    return rollover;
}

bool TimerManager::hasTimer() {
    RWMutexType::ReadLock lock(m_mutex);
    return !m_timers.empty();
}

}
