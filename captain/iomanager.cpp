#include "include/iomanager.h"
#include "include/macro.h"
#include "include/log.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <string.h>
#include <unistd.h>

namespace captain {

static captain::Logger::ptr g_logger = CAPTAIN_LOG_NAME("system");

//根据给定的事件类型，返回相应的事件上下文
IOManager::FdContext::EventContext& IOManager::FdContext::getContext(IOManager::Event event) {
    switch(event) {
        case IOManager::READ:
            return read;
        case IOManager::WRITE:
            return write;
        default:
            CAPTAIN_ASSERT2(false, "getContext");
    }
}

//重置指定的事件上下文。
void IOManager::FdContext::resetContext(EventContext& ctx) {
    ctx.scheduler = nullptr;
    ctx.fiber.reset();
    ctx.cb = nullptr;
}

void IOManager::FdContext::triggerEvent(IOManager::Event event) {
    CAPTAIN_ASSERT(events & event); //断言指定的事件在事件集合中
    events = (Event)(events & ~event); //从事件集合中移除特定事件
    EventContext& ctx = getContext(event); //获取指定事件的上下文
    if(ctx.cb) {
        //有回调函数，则使用调度器将回调函数提交到调度器进行执行
        ctx.scheduler->schedule(&ctx.cb);
    } else {
        //使用调度器将关联的协程提交到调度器进行执行。
        ctx.scheduler->schedule(&ctx.fiber);
    }
    //清空事件上下文中的调度器，以避免出现悬挂的情况。
    ctx.scheduler = nullptr;
    return;
}

IOManager::IOManager(size_t threads, bool use_caller, const std::string& name)
    :Scheduler(threads, use_caller, name) {
    m_epfd = epoll_create(5000);
    CAPTAIN_ASSERT(m_epfd > 0);
    //创建一个管道，用于唤醒事件循环线程。m_tickleFds 数组保存管道两端的文件描述符
    int rt = pipe(m_tickleFds);
    CAPTAIN_ASSERT(!rt);

    epoll_event event;
    memset(&event, 0, sizeof(epoll_event)); //初始化
    event.events = EPOLLIN | EPOLLET;  //边缘触发模式  只通知一次 要一次性处理完所有数据
    event.data.fd = m_tickleFds[0];//监听管道的读端文件描述符 m_tickleFds[0] 上的可读事件。

    //设置管道读端为非阻塞模式，以确保读操作不会阻塞。  fcntl 函数用于获取或设置文件描述符的属性
    //O_NONBLOCK 表示非阻塞模式
    rt = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);
    CAPTAIN_ASSERT(!rt);

    rt = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &event);
    CAPTAIN_ASSERT(!rt);

    //初始化文件描述符上下文数组
    contextResize(32);
    
    start(); //Scheduler::start  创建好了就默认启动
}

IOManager::~IOManager() {
    stop(); //Scheduler::stop 
    close(m_epfd); //关闭 epoll 文件描述符
    close(m_tickleFds[0]); //关闭管道描述符
    close(m_tickleFds[1]);
    //释放内存
    for(size_t i = 0; i < m_fdContexts.size(); ++i) {
        if(m_fdContexts[i]) { //如果某个上下文存在（不为 nullptr），则释放其内存。
            delete m_fdContexts[i];
        }
    }
}

//确保 m_fdContexts 数组能够容纳指定大小的句柄上下文对象，并在需要时创建新的对象。
void IOManager::contextResize(size_t size) {
    m_fdContexts.resize(size);

    for(size_t i = 0; i < m_fdContexts.size(); ++i) {
        if(!m_fdContexts[i]) { //创建新的句柄上下文
            m_fdContexts[i] = new FdContext;
            m_fdContexts[i]->fd = i; //将句柄上下文的 fd 属性设置为当前数组索引值 i
        }
    }
}

int IOManager::addEvent(int fd, Event event, std::function<void()> cb) {
    FdContext* fd_ctx = nullptr;
    RWMutexType::ReadLock lock(m_mutex);
    if((int)m_fdContexts.size() > fd) {
        fd_ctx = m_fdContexts[fd];
        lock.unlock();
    } else {
        lock.unlock();
        RWMutexType::WriteLock lock2(m_mutex);
        contextResize(fd * 1.5);
        fd_ctx = m_fdContexts[fd];
    }

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    //检查是否已经注册了相同的事件
    if(fd_ctx->events & event) {
        CAPTAIN_LOG_ERROR(g_logger) << "addEvent assert fd=" << fd
                    << " event=" << event
                    << " fd_ctx.event=" << fd_ctx->events;
        CAPTAIN_ASSERT(!(fd_ctx->events & event));
    }
    //根据 fd_ctx->events 的值判断是要添加新事件还是修改已有事件
    int op = fd_ctx->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
    epoll_event epevent; //用于设置要注册的事件的相关属性
    //EPOLLET 表示使用边缘触发模式；fd_ctx->events 表示已经注册的事件；event 表示待添加或修改的事件。
    epevent.events = EPOLLET | fd_ctx->events | event;
    epevent.data.ptr = fd_ctx;
    //向 epoll 实例注册或修改事件 失败：返回-1  成功：返回0
    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if(rt) {
        CAPTAIN_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
            << op << "," << fd << "," << epevent.events << "):"
            << rt << " (" << errno << ") (" << strerror(errno) << ")";
        return -1;
    }

    ++m_pendingEventCount; //有一个新的事件要被处理。
    fd_ctx->events = (Event)(fd_ctx->events | event); //将新事件添加到已有的事件集合中。
    FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
    //确保在添加事件时，事件上下文是空的
    CAPTAIN_ASSERT(!event_ctx.scheduler
                && !event_ctx.fiber
                && !event_ctx.cb);

    event_ctx.scheduler = Scheduler::GetThis();
    if(cb) {  //如果传入的回调函数 cb 不为空，将其与事件上下文中的回调函数进行交换。这意味着当事件就绪时，将执行这个回调函数。
        event_ctx.cb.swap(cb);
    } else {
        //将当前协程（Fiber::GetThis()）设置为事件上下文的协程。
        event_ctx.fiber = Fiber::GetThis();
        //使用断言确保当前协程的状态为 正在执行状态。
        CAPTAIN_ASSERT(event_ctx.fiber->getState() == Fiber::EXEC);
    }
    return 0; //0 success, -1 error
}

//从文件描述符上下文中删除一个已注册的事件
bool IOManager::delEvent(int fd, Event event) {
    RWMutexType::ReadLock lock(m_mutex);
    if((int)m_fdContexts.size() <= fd) {
        return false;
    }
    //获取文件描述符上下文（FdContext）的指针
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    //fd_ctx->events 表示文件描述符上已经注册的事件集合  event表示要删除的事件。
    if(!(fd_ctx->events & event)) { //如果结果为 true，则表示要删除的事件尚未在文件描述符上注册，所以函数直接返回 false，表示删除事件失败。
        return false;
    }
    //计算出新的事件集合，其中排除了要删除的事件。
    Event new_events = (Event)(fd_ctx->events & ~event);
    //根据新的事件集合是否为空，决定采用 EPOLL_CTL_MOD 还是 EPOLL_CTL_DEL 操作。
    int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = EPOLLET | new_events;
    epevent.data.ptr = fd_ctx;
    //使用 epoll_ctl 函数执行删除或修改操作。
    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if(rt) {
        CAPTAIN_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
            << op << "," << fd << "," << epevent.events << "):"
            << rt << " (" << errno << ") (" << strerror(errno) << ")";
        return false;
    }
    //在删除事件后对相应的数据进行更新和重置
    --m_pendingEventCount;
    fd_ctx->events = new_events; //已删除特定事件后的事件集合
    FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
    fd_ctx->resetContext(event_ctx); //重置特定事件event的事件上下文
    return true;
}

bool IOManager::cancelEvent(int fd, Event event) {
    RWMutexType::ReadLock lock(m_mutex);
    if((int)m_fdContexts.size() <= fd) {
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if(!(fd_ctx->events & event)) { //检查要取消的事件是否存在
        return false;
    }
    //计算取消特定事件后的新事件集合。
    Event new_events = (Event)(fd_ctx->events & ~event);
    int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = EPOLLET | new_events;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if(rt) {
        CAPTAIN_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
            << op << "," << fd << "," << epevent.events << "):"
            << rt << " (" << errno << ") (" << strerror(errno) << ")";
        return false;
    }

    fd_ctx->triggerEvent(event);
    --m_pendingEventCount;
    return true;
}

//取消一个句柄上的所有事件的操作
bool IOManager::cancelAll(int fd) {
    RWMutexType::ReadLock lock(m_mutex);
    if((int)m_fdContexts.size() <= fd) {
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if(!fd_ctx->events) { //如果句柄没有任何事件，直接返回 false
        return false;
    }

    int op = EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = 0; //删除该句柄上的所有事件。
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if(rt) {
        CAPTAIN_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
            << op << "," << fd << "," << epevent.events << "):"
            << rt << " (" << errno << ") (" << strerror(errno) << ")";
        return false;
    }
    //取消指定句柄上的所有事件，并触发已注册事件的回调或协程
    if(fd_ctx->events & READ) {
        fd_ctx->triggerEvent(READ);
        --m_pendingEventCount;
    }
    if(fd_ctx->events & WRITE) {
        fd_ctx->triggerEvent(WRITE);
        --m_pendingEventCount;
    }
    //断言句柄上的事件已全部取消，即 fd_ctx->events 应为 0。
    CAPTAIN_ASSERT(fd_ctx->events == 0);
    return true;
}

IOManager* IOManager::GetThis() {
    //尝试将调度器实例指针转换为 IOManager* 类型。
    //如果实际的调度器类型是 IOManager 或其派生类，转换会成功，返回 IOManager* 类型的指针；否则，返回 nullptr。
    return dynamic_cast<IOManager*>(Scheduler::GetThis());
}

void IOManager::tickle() {
    if(hasIdleThreads()) { //检查是否有空闲的线程
        //有空闲线程，说明有线程可以执行任务，所以函数直接返回，不执行后续的唤醒操作。
        return;
    }
    //写入一个字节到管道中的方式，唤醒其中一个 IO 线程
    int rt = write(m_tickleFds[1], "T", 1);
    CAPTAIN_ASSERT(rt == 1);
}

//如果没有定时器、没有待处理事件，而且调度器也正在停止，那么函数返回 true，表示 IO 管理器应该停止。
bool IOManager::stopping(uint64_t& timeout) {
    timeout = getNextTimer();
    //~0ull 表示无限大的超时时间，即没有定时器。如果没有定时器，说明不需要等待定时器触发，可以继续执行。
    return timeout == ~0ull
        && m_pendingEventCount == 0
        && Scheduler::stopping();

}

bool IOManager::stopping() {
    uint64_t timeout = 0;
    return stopping(timeout);
}

void IOManager::idle() {
    //分配一个大小为 64 的 epoll_event 数组
    epoll_event* events = new epoll_event[64]();
    //使用 std::shared_ptr 来确保在函数结束时自动释放内存。
    //第二个参数是一个 lambda 函数，用于在共享指针的引用计数归零时执行释放操作
    std::shared_ptr<epoll_event> shared_events(events, [](epoll_event* ptr){
        delete[] ptr;
    });

    while(true) {
        uint64_t next_timeout = 0;
        if(stopping(next_timeout)) {
            CAPTAIN_LOG_INFO(g_logger) << "name=" << getName()
                                     << " idle stopping exit";
            break;
        }

        int rt = 0;
        do {
            static const int MAX_TIMEOUT = 3000; //用于限制最大的等待时间
            if(next_timeout != ~0ull) {
                //取较小的那个值作为等待时间。
                next_timeout = (int)next_timeout > MAX_TIMEOUT
                                ? MAX_TIMEOUT : next_timeout;
            } else {
                //没有定时器事件
                next_timeout = MAX_TIMEOUT;
            }
            rt = epoll_wait(m_epfd, events, 64, (int)next_timeout);
            //如果rt < 0 && errno == EINTR，表示在等待过程中被中断，这种情况下不需要处理，直接继续下一次循环。否则，就是等待过程正常结束，可以退出循环。
            if(rt < 0 && errno == EINTR) {
            } else {
                break;
            }
        } while(true);

        //处理已经过期的定时器回调任务
        std::vector<std::function<void()> > cbs;
        listExpiredCb(cbs);
        if(!cbs.empty()) {
            //CAPTAIN_LOG_DEBUG(g_logger) << "on timer cbs.size=" << cbs.size();
            //这些过期任务放入调度队列，以便后续调度执行
            schedule(cbs.begin(), cbs.end());
            cbs.clear();
        }

        for(int i = 0; i < rt; ++i) { //循环遍历就绪的事件数组 events，rt 表示就绪的事件数量。
            epoll_event& event = events[i];
            if(event.data.fd == m_tickleFds[0]) { //m_tickleFds[0]，则说明这是管道的读端（用于通知唤醒）的事件。
                uint8_t dummy;
                while(read(m_tickleFds[0], &dummy, 1) == 1);
                continue; //读到没有数据就继续下面
            }
            //处理非管道事件
            FdContext* fd_ctx = (FdContext*)event.data.ptr;
            FdContext::MutexType::Lock lock(fd_ctx->mutex);
            //如果事件的标志中包含了 EPOLLERR 或 EPOLLHUP，说明发生了错误或挂起事件
            if(event.events & (EPOLLERR | EPOLLHUP)) {
                //将 EPOLLIN 和 EPOLLOUT 事件也加入到 event.events 标志中，以确保错误或挂起事件被同时处理。
                //这样做的原因是，当发生错误或挂起事件时，通常也需要读取或写入数据来清除错误状态。通过将 EPOLLIN 和 
                //EPOLLOUT 事件加入到事件标志中，确保了在处理错误或挂起事件时，也能够正确地读取或写入数据，
                //以便将文件描述符的状态恢复到正常。
                event.events |= EPOLLIN | EPOLLOUT;
            }
            //根据 event.events 的值，将实际的事件类型存储在 real_events 变量中
            int real_events = NONE;
            //如果 event.events 包含 EPOLLIN 标志，就将 READ 事件添加到 real_events 中
            if(event.events & EPOLLIN) {
                real_events |= READ;
            }
            if(event.events & EPOLLOUT) {
                real_events |= WRITE;
            }
            
            if((fd_ctx->events & real_events) == NONE) {//判断当前事件类型是否在文件描述符上下文的事件集合中
                continue;
            }
            //首先计算出剩余关注的事件类型，即从 fd_ctx->events 中排除掉当前已经触发的 real_events。
            int left_events = (fd_ctx->events & ~real_events);
            //根据剩余的事件类型，判断是应该修改还是删除当前文件描述符上的事件监听。
            int op = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
            event.events = EPOLLET | left_events;

            int rt2 = epoll_ctl(m_epfd, op, fd_ctx->fd, &event);
            if(rt2) {
                CAPTAIN_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                    << op << "," << fd_ctx->fd << "," << event.events << "):"
                    << rt2 << " (" << errno << ") (" << strerror(errno) << ")";
                continue;
            }

            if(real_events & READ) {
                fd_ctx->triggerEvent(READ);
                --m_pendingEventCount;
            }
            if(real_events & WRITE) {
                fd_ctx->triggerEvent(WRITE);
                --m_pendingEventCount;
            }
        }
        
        //在协程调度的框架下，实现协程的切换，从而实现多个协程在单线程中并发执行。
        //让出执行权
        Fiber::ptr cur = Fiber::GetThis();
        auto raw_ptr = cur.get();
        cur.reset();

        raw_ptr->swapOut();
    }
}

void IOManager::onTimerInsertedAtFront() {
    tickle();
}

}
