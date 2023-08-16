#pragma once

#include "scheduler.h"
#include "timer.h"

namespace captain {

class IOManager : public Scheduler, public TimerManager {
public:
    typedef std::shared_ptr<IOManager> ptr;
    typedef RWMutex RWMutexType;

    enum Event {
        NONE    = 0x0,
        READ    = 0x1, //EPOLLIN  读事件
        WRITE   = 0x4, //EPOLLOUT 写事件
    };
private:
    struct FdContext {
        typedef Mutex MutexType;
        struct EventContext {
            Scheduler* scheduler = nullptr; //事件执行的scheduler
            Fiber::ptr fiber;               //事件协程
            std::function<void()> cb;       //事件的回调函数
        };

        EventContext& getContext(Event event);
        void resetContext(EventContext& ctx);
        void triggerEvent(Event event);

        EventContext read;      //读事件
        EventContext write;     //写事件
        int fd = 0;             //事件关联的句柄
        Event events = NONE;    //已经注册的事件
        MutexType mutex;
    };

public:
    //IOManager构造函数的签名和Scheduler的一样
    IOManager(size_t threads = 1, bool use_caller = true, const std::string& name = "");
    ~IOManager();

    //0 success, -1 error
    int addEvent(int fd, Event event, std::function<void()> cb = nullptr);
    bool delEvent(int fd, Event event);
    bool cancelEvent(int fd, Event event);

    bool cancelAll(int fd); //把一个句柄上的所有事件都取消

    static IOManager* GetThis(); //获取当前的IOManager

protected:
    //实现Scheduler里的三个虚方法
    void tickle() override;
    bool stopping() override;
    void idle() override;

    void onTimerInsertedAtFront() override;

    void contextResize(size_t size);
    bool stopping(uint64_t& timeout);
private:
    int m_epfd = 0;    //epoll 的 fd
    //一个包含两个整数的数组，用于创建一个管道，用于唤醒事件循环线程
    //其中一个文件描述符用于写入数据（m_tickleFds[0]），另一个用于读取数据（m_tickleFds[1]）。
    int m_tickleFds[2]; //pipe的一个传入参数

    std::atomic<size_t> m_pendingEventCount = {0};  //正在等待执行的事件数量
    RWMutexType m_mutex;
    std::vector<FdContext*> m_fdContexts;  //用于存放文件描述符的上下文信息
};

}

