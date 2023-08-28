#include "include/hook.h"
#include <dlfcn.h>

#include "include/config.h"
#include "include/log.h"
#include "include/fiber.h"
#include "include/iomanager.h"
#include "include/fd_manager.h"

captain::Logger::ptr g_logger = CAPTAIN_LOG_NAME("system");
namespace captain {

static captain::ConfigVar<int>::ptr g_tcp_connect_timeout =
    captain::Config::Lookup("tcp.connect.timeout", 5000, "tcp connect timeout");

static thread_local bool t_hook_enable = false;

#define HOOK_FUN(XX) \
    XX(sleep) \
    XX(usleep) \
    XX(nanosleep) \
    XX(socket) \
    XX(connect) \
    XX(accept) \
    XX(read) \
    XX(readv) \
    XX(recv) \
    XX(recvfrom) \
    XX(recvmsg) \
    XX(write) \
    XX(writev) \
    XX(send) \
    XX(sendto) \
    XX(sendmsg) \
    XX(close) \
    XX(fcntl) \
    XX(ioctl) \
    XX(getsockopt) \
    XX(setsockopt)

void hook_init() {
    static bool is_inited = false;
    if(is_inited) {
        return;
    }
//用 dlsym 函数来获取原始系统函数的地址，将该地址赋值给函数指针。
#define XX(name) name ## _f = (name ## _fun)dlsym(RTLD_NEXT, #name);
    HOOK_FUN(XX);
#undef XX
}

static uint64_t s_connect_timeout = -1;
struct _HookIniter {
    _HookIniter() {
        hook_init();
        s_connect_timeout = g_tcp_connect_timeout->getValue();

        g_tcp_connect_timeout->addListener([](const int& old_value, const int& new_value){
                CAPTAIN_LOG_INFO(g_logger) << "tcp connect timeout changed from "
                                         << old_value << " to " << new_value;
                s_connect_timeout = new_value;
        });
    }
};

static _HookIniter s_hook_initer;

bool is_hook_enable() {
    return t_hook_enable;
}

void set_hook_enable(bool flag) {
    t_hook_enable = flag;
}

}

struct timer_info {
    int cancelled = 0;
};

//do_io：hook住和io相关的一些操作
template<typename OriginFun, typename... Args>
static ssize_t do_io(int fd, OriginFun fun, const char* hook_fun_name,
        uint32_t event, int timeout_so, Args&&... args) {
    //没有被hook  直接执行原函数
    if(!captain::t_hook_enable) {
        return fun(fd, std::forward<Args>(args)...);
    }

    //文件句柄不存在，则就不是socket操作相关的句柄
    captain::FdCtx::ptr ctx = captain::FdMgr::GetInstance()->get(fd);
    if(!ctx) {
        return fun(fd, std::forward<Args>(args)...);
    }

    //是文件句柄  但被关闭了
    if(ctx->isClose()) {
        errno = EBADF;
        return -1;
    }

    //不是socket 或者 用户自己设置了Nonblock， 执行原来的函数
    if(!ctx->isSocket() || ctx->getUserNonblock()) {
        return fun(fd, std::forward<Args>(args)...);
    }

//自己做的操作
    /* 【条件】 */
    //to 超时时间
    uint64_t to = ctx->getTimeout(timeout_so);
    std::shared_ptr<timer_info> tinfo(new timer_info);

retry:
    //先尝试直接执行，如果能返回一个非负数。说明以及读到数据了？可以直接return出去
    ssize_t n = fun(fd, std::forward<Args>(args)...);
    //如果n == -1 && errno == EINTR  就继续重试
    while(n == -1 && errno == EINTR) {
        n = fun(fd, std::forward<Args>(args)...);
    }
    //重试之后，如果n == -1 而且错误变为errno == EAGAIN，那就需要做【异步操作】
    if(n == -1 && errno == EAGAIN) {
        //CAPTAIN_LOG_DEBUG(g_logger) << "do_io<" << hook_fun_name << ">";
        //拿出当前线程所在的iomanager
        captain::IOManager* iom = captain::IOManager::GetThis();
        captain::Timer::ptr timer; //定时器，一个定时任务的类
        //【条件】变量  设置在了上面
        std::weak_ptr<timer_info> winfo(tinfo);

        //如果超时时间不等于-1，则说明设置了超时
        if(to != (uint64_t)-1) {
            //首先必须把超时放到定时器中 等待一段时间还没来的话  就触发
            timer = iom->addConditionTimer(to, [winfo, fd, iom, event]() {
                //触发定时器
                auto t = winfo.lock();
                //定时器取出【条件】，如果【条件】不存在或已经被取消，就return
                if(!t || t->cancelled) {
                    return;
                }
                //否则设置其取消事件为ETIMEDOUT
                t->cancelled = ETIMEDOUT;
                //把io类型的事件取消掉
                iom->cancelEvent(fd, (captain::IOManager::Event)(event));
            }, winfo);
        }
        //取消掉之后 会从event中唤醒回来？ 且 没带回调函数 会用默认用当前协程做回调参数
        int rt = iom->addEvent(fd, (captain::IOManager::Event)(event));
        //如果添加失败
        if(rt) {
            CAPTAIN_LOG_ERROR(g_logger) << hook_fun_name << " addEvent("
                << fd << ", " << event << ")";
            if(timer) {
                timer->cancel(); //取消定时器
            }
            return -1;
        } else {
            //CAPTAIN_LOG_DEBUG(g_logger) << "do_io<" << hook_fun_name << ">";
            //成功，让出当前协程的执行时间
            captain::Fiber::YieldToHold();
            //CAPTAIN_LOG_DEBUG(g_logger) << "do_io<" << hook_fun_name << ">";
            if(timer) {
                timer->cancel();
            }
            //如果info->cancelled，说明是通过定时任务将其唤醒的  说明已经超时
            if(tinfo->cancelled) {
                errno = tinfo->cancelled;
                return -1;
            }
            //如果事件回来了  再重新读
            goto retry;
        }
    }
    
    return n;
}


extern "C" {
#define XX(name) name ## _fun name ## _f = nullptr;
    HOOK_FUN(XX);
#undef XX

unsigned int sleep(unsigned int seconds) {
    //如果 captain::t_hook_enable 为假，那么直接调用原始的 sleep 函数
    if(!captain::t_hook_enable) {
        return sleep_f(seconds);
    }
    //如果 captain::t_hook_enable 为真，说明启用了钩子，此时会将当前协程挂起，等待指定的时间后恢复
    captain::Fiber::ptr fiber = captain::Fiber::GetThis();
    captain::IOManager* iom = captain::IOManager::GetThis();
    iom->addTimer(seconds * 1000, std::bind((void(captain::Scheduler::*)
            (captain::Fiber::ptr, int thread))&captain::IOManager::schedule
            ,iom, fiber, -1));
    captain::Fiber::YieldToHold();
    return 0;
}

int usleep(useconds_t usec) {
    if(!captain::t_hook_enable) {
        return usleep_f(usec);
    }
    captain::Fiber::ptr fiber = captain::Fiber::GetThis();
    captain::IOManager* iom = captain::IOManager::GetThis();
    iom->addTimer(usec / 1000, std::bind((void(captain::Scheduler::*)
            (captain::Fiber::ptr, int thread))&captain::IOManager::schedule
            ,iom, fiber, -1));
    captain::Fiber::YieldToHold();
    return 0;
}

int nanosleep(const struct timespec *req, struct timespec *rem) {
    if(!captain::t_hook_enable) {
        return nanosleep_f(req, rem);
    }

    int timeout_ms = req->tv_sec * 1000 + req->tv_nsec / 1000 /1000;
    captain::Fiber::ptr fiber = captain::Fiber::GetThis();
    captain::IOManager* iom = captain::IOManager::GetThis();
    iom->addTimer(timeout_ms, std::bind((void(captain::Scheduler::*)
            (captain::Fiber::ptr, int thread))&captain::IOManager::schedule
            ,iom, fiber, -1));
    captain::Fiber::YieldToHold();
    return 0;
}

int socket(int domain, int type, int protocol) {
    if(!captain::t_hook_enable) {
        return socket_f(domain, type, protocol);
    }
    int fd = socket_f(domain, type, protocol);
    if(fd == -1) {
        return fd;
    }
    captain::FdMgr::GetInstance()->get(fd, true);
    return fd;
}

int connect_with_timeout(int fd, const struct sockaddr* addr, socklen_t addrlen, uint64_t timeout_ms) {
    if(!captain::t_hook_enable) {
        return connect_f(fd, addr, addrlen);
    }
    //获取与文件描述符 fd 关联的 captain::FdCtx 对象
    captain::FdCtx::ptr ctx = captain::FdMgr::GetInstance()->get(fd);
    //如果该对象不存在或已关闭，则设置错误码 EBADF 并返回 -1。
    if(!ctx || ctx->isClose()) {
        errno = EBADF;
        return -1;
    }
    //检查文件描述符是否为套接字，如果不是套接字，则同样直接调用原始的 connect_f 函数。
    if(!ctx->isSocket()) {
        return connect_f(fd, addr, addrlen);
    }

    //如果是套接字，并设置了非阻塞标志，则同样直接调用原始的 connect_f 函数。
    if(ctx->getUserNonblock()) {
        return connect_f(fd, addr, addrlen);
    }
    //如果不满足上述条件，它尝试调用原始的 connect_f 函数进行连接操作。
    int n = connect_f(fd, addr, addrlen);
    if(n == 0) { //如果连接操作返回0，表示连接成功，直接返回0。
        return 0;
    } else if(n != -1 || errno != EINPROGRESS) {
        //如果返回值不是-1，但错误码不是 EINPROGRESS，则返回这个非-1的返回值
        return n;
    }
    //如果返回值是-1 并且错误码是 EINPROGRESS，表示连接正在进行中，这时它创建一个定时器，用于在指定的超时时间后取消连接操作。
    //取当前的 captain::IOManager 实例。
    captain::IOManager* iom = captain::IOManager::GetThis();
    //准备一个用于超时取消的定时器和相关信息。
    captain::Timer::ptr timer;
    std::shared_ptr<timer_info> tinfo(new timer_info);
    std::weak_ptr<timer_info> winfo(tinfo);

    if(timeout_ms != (uint64_t)-1) { //(uint64_t)-1 表示无穷大的超时时间，即不设置超时。
        //如果设置了超时时间 timeout_ms，则创建一个条件定时器，当定时器超时时，会执行指定的回调函数。
        timer = iom->addConditionTimer(timeout_ms, [winfo, fd, iom]() {
                /* 
                回调函数首先检查定时器信息是否有效，以及是否已被取消，如果是，则不执行任何操作。
                如果未取消，则将标志设置为 ETIMEDOUT，然后取消写事件。
                 */
                auto t = winfo.lock();
                if(!t || t->cancelled) {
                    return;
                }
                t->cancelled = ETIMEDOUT;
                iom->cancelEvent(fd, captain::IOManager::WRITE);
        }, winfo);
    }
    //尝试将套接字的写事件注册到 IOManager 上。
    int rt = iom->addEvent(fd, captain::IOManager::WRITE);
    //如果写事件注册成功（返回值为0），则当前协程切换到其他协程。这样，其他协程就有机会继续执行，而不会被当前协程阻塞。
    //同时，定时器也被取消，以防止后续不必要的超时处理。
    if(rt == 0) {
        captain::Fiber::YieldToHold();
        if(timer) {
            timer->cancel();
        }
        if(tinfo->cancelled) {
            errno = tinfo->cancelled;
            return -1;
        }
    } else { //如果写事件注册失败，即 addEvent 返回非零值，那么同样需要取消定时器，然后记录错误日志。
        if(timer) {
            timer->cancel();
        }
        CAPTAIN_LOG_ERROR(g_logger) << "connect addEvent(" << fd << ", WRITE) error";
    }

    int error = 0;
    socklen_t len = sizeof(int);
    //调用 getsockopt 函数来获取套接字上的错误信息。在这里，SO_ERROR 是一个套接字选项，用于获取上次套接字操作失败的错误码。
    //使用 getsockopt 函数来获取 SO_ERROR 的值，存储在 error 变量中
    if(-1 == getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len)) {
        return -1;
    }
    //如果调用成功，且 error 为0，意味着连接操作没有出现错误，可以返回0表示成功。
    if(!error) {
        return 0;
    } else {//如果 error 不为0，意味着连接操作出现了错误，此时将设置 errno 为对应的错误码，并返回-1表示连接失败。
        errno = error;
        return -1;
    }
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    return connect_with_timeout(sockfd, addr, addrlen, captain::s_connect_timeout);
}

int accept(int s, struct sockaddr *addr, socklen_t *addrlen) {
    int fd = do_io(s, accept_f, "accept", captain::IOManager::READ, SO_RCVTIMEO, addr, addrlen);
    if(fd >= 0) {
        captain::FdMgr::GetInstance()->get(fd, true);
    }
    return fd;
}

ssize_t read(int fd, void *buf, size_t count) {
    return do_io(fd, read_f, "read", captain::IOManager::READ, SO_RCVTIMEO, buf, count);
}

ssize_t readv(int fd, const struct iovec *iov, int iovcnt) {
    return do_io(fd, readv_f, "readv", captain::IOManager::READ, SO_RCVTIMEO, iov, iovcnt);
}

ssize_t recv(int sockfd, void *buf, size_t len, int flags) {
    return do_io(sockfd, recv_f, "recv", captain::IOManager::READ, SO_RCVTIMEO, buf, len, flags);
}

ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen) {
    return do_io(sockfd, recvfrom_f, "recvfrom", captain::IOManager::READ, SO_RCVTIMEO, buf, len, flags, src_addr, addrlen);
}

ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags) {
    return do_io(sockfd, recvmsg_f, "recvmsg", captain::IOManager::READ, SO_RCVTIMEO, msg, flags);
}

ssize_t write(int fd, const void *buf, size_t count) {
    return do_io(fd, write_f, "write", captain::IOManager::WRITE, SO_SNDTIMEO, buf, count);
}

ssize_t writev(int fd, const struct iovec *iov, int iovcnt) {
    return do_io(fd, writev_f, "writev", captain::IOManager::WRITE, SO_SNDTIMEO, iov, iovcnt);
}

ssize_t send(int s, const void *msg, size_t len, int flags) {
    return do_io(s, send_f, "send", captain::IOManager::WRITE, SO_SNDTIMEO, msg, len, flags);
}

ssize_t sendto(int s, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen) {
    return do_io(s, sendto_f, "sendto", captain::IOManager::WRITE, SO_SNDTIMEO, msg, len, flags, to, tolen);
}

ssize_t sendmsg(int s, const struct msghdr *msg, int flags) {
    return do_io(s, sendmsg_f, "sendmsg", captain::IOManager::WRITE, SO_SNDTIMEO, msg, flags);
}

int close(int fd) {
    if(!captain::t_hook_enable) {
        return close_f(fd);
    }

    captain::FdCtx::ptr ctx = captain::FdMgr::GetInstance()->get(fd);
    if(ctx) {
        auto iom = captain::IOManager::GetThis();
        if(iom) {
            iom->cancelAll(fd);
        }
        captain::FdMgr::GetInstance()->del(fd);
    }
    return close_f(fd);
}

/* 
重写了 fcntl 函数，通过钩子技术来实现对原始的 fcntl 函数的调用重定向。
在重写的函数内部，根据不同的 cmd 参数，执行不同的逻辑
 */
int fcntl(int fd, int cmd, ... /* arg */ ) {
    va_list va; //存储可变参数列表
    //va_start用于初始化 va_list 对象，以便后续可以使用 va_arg 来获取可变参数列表中的值。
    va_start(va, cmd);
    switch(cmd) {
        case F_SETFL:
            {
                //获取参数 arg，这是用户传递给 fcntl 的操作标志，例如 O_NONBLOCK。
                int arg = va_arg(va, int);
                va_end(va);
                //通过文件描述符管理器（FdMgr）获取文件描述符的上下文（FdCtx）
                captain::FdCtx::ptr ctx = captain::FdMgr::GetInstance()->get(fd);
                //如果文件描述符的上下文不存在，已关闭，或者不是套接字类型，就直接调用原始的 fcntl 函数，以保持默认行为。
                if(!ctx || ctx->isClose() || !ctx->isSocket()) {
                    return fcntl_f(fd, cmd, arg);
                }
                //如果上下文存在，设置上下文的用户自定义非阻塞标志
                ctx->setUserNonblock(arg & O_NONBLOCK);
                //根据上下文的系统非阻塞标志来调整 arg
                if(ctx->getSysNonblock()) {
                    arg |= O_NONBLOCK;
                } else {
                    arg &= ~O_NONBLOCK;
                }
                //调用原始的 fcntl 函数，将修改后的 arg 应用于文件描述符的属性设置。
                return fcntl_f(fd, cmd, arg);
            }
            break;
        case F_GETFL:
            {
                va_end(va);
                //获取文件描述符的属性值（arg）通过调用原始的 fcntl 函数，这里传递的是 F_GETFL 命令。
                int arg = fcntl_f(fd, cmd);
                captain::FdCtx::ptr ctx = captain::FdMgr::GetInstance()->get(fd);
                if(!ctx || ctx->isClose() || !ctx->isSocket()) {
                    return arg;//返回原始的属性值 arg。
                }
                if(ctx->getUserNonblock()) {
                    return arg | O_NONBLOCK;
                } else {
                    return arg & ~O_NONBLOCK;
                }
            }
            break;
        case F_DUPFD:
        case F_DUPFD_CLOEXEC:
        case F_SETFD:
        case F_SETOWN:
        case F_SETSIG:
        case F_SETLEASE:
        case F_NOTIFY:
        case F_SETPIPE_SZ:
            {
                //获取了可变参数列表中的一个整数值，这个整数值即为命令 F_SETPIPE_SZ 所需要的参数
                int arg = va_arg(va, int);
                va_end(va);
                //调用原始的 fcntl 函数，传入了之前获取到的参数。
                return fcntl_f(fd, cmd, arg); 
            }
            break;
        case F_GETFD:
        case F_GETOWN:
        case F_GETSIG:
        case F_GETLEASE:
        case F_GETPIPE_SZ:
            {
                //结束可变参数列表的使用，因为在这个 case 中不会再使用这个可变参数列表。
                va_end(va);
                return fcntl_f(fd, cmd);
            }
            break;
        case F_SETLK:
        case F_SETLKW:
        case F_GETLK:
            {
                struct flock* arg = va_arg(va, struct flock*);
                va_end(va);
                //调用了原始的 fcntl 函数，传入了这个 struct flock* 参数，以实现对 F_GETLK 命令的处理，同时完成了对原始函数的调用。
                return fcntl_f(fd, cmd, arg);
            }
            break;
        case F_GETOWN_EX:
        case F_SETOWN_EX:
            {
                struct f_owner_exlock* arg = va_arg(va, struct f_owner_exlock*);
                va_end(va);
                return fcntl_f(fd, cmd, arg);
            }
            break;
        default:  //对于未特别处理的 cmd 命令，直接调用原始函数进行处理。
            va_end(va);
            return fcntl_f(fd, cmd);
    }
}

//实现 ioctl 函数的 hook
int ioctl(int d, unsigned long int request, ...) {
    va_list va;
    va_start(va, request);
    void* arg = va_arg(va, void*);
    va_end(va);
    /*
    使用 FIONBIO 可以通过将一个整数值作为参数传递给 ioctl 函数，以设置或获取文件描述符的非阻塞状态。
      */
    if(FIONBIO == request) {
        //获取 arg 中的值（指针解引用），将其转换为布尔值，判断是否为非零，以确定用户是否希望设置为非阻塞模式
        bool user_nonblock = !!*(int*)arg;
        //获取文件描述符 d 对应的 FdCtx 对象
        captain::FdCtx::ptr ctx = captain::FdMgr::GetInstance()->get(d);
        //如果 FdCtx 不存在、已关闭或者不是套接字，那么它会调用原始的 ioctl 函数来处理。
        if(!ctx || ctx->isClose() || !ctx->isSocket()) {
            return ioctl_f(d, request, arg);
        }
        //如果 FdCtx 存在且没有关闭且是套接字，它将设置 FdCtx 对象的用户自定义非阻塞标志。
        ctx->setUserNonblock(user_nonblock);
    }
    //最后，不论是否是设置非阻塞模式，都会调用原始的 ioctl 函数来执行实际的操作。
    return ioctl_f(d, request, arg);
}

int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen) {
    return getsockopt_f(sockfd, level, optname, optval, optlen);
}

int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen) {
    if(!captain::t_hook_enable) {
        //调用委托给原始的 setsockopt_f 函数，以保持原始函数的行为。
        return setsockopt_f(sockfd, level, optname, optval, optlen);
    }
    if(level == SOL_SOCKET) {
        //SO_RCVTIMEO、SO_SNDTIMEO这两个选项用于设置接收和发送超时时间。
        if(optname == SO_RCVTIMEO || optname == SO_SNDTIMEO) {
            captain::FdCtx::ptr ctx = captain::FdMgr::GetInstance()->get(sockfd);
            if(ctx) {
                //代码从 optval 中提取出时间值
                const timeval* v = (const timeval*)optval;
                //然后根据秒和微秒计算出总的毫秒超时时间，并将这个超时时间设置到与套接字关联的 captain::FdCtx 对象中。
                ctx->setTimeout(optname, v->tv_sec * 1000 + v->tv_usec / 1000);
            }
        }
    }
    //最终，无论是否设置了超时时间，都将调用委托给原始的 setsockopt_f 函数，以实现实际的套接字选项设置操作。
    return setsockopt_f(sockfd, level, optname, optval, optlen);
}

}
