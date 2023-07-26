#include "include/thread.h"
#include "include/log.h"
#include "include/util.h"

namespace captain {

//定义线程局部变量
static thread_local Thread* t_thread = nullptr; //指向当前线程
static thread_local std::string t_thread_name = "UNKNOW"; //当前线程名称
//系统日志
static captain::Logger::ptr g_logger = CAPTAIN_LOG_NAME("system");



Thread* Thread::GetThis() {
    return t_thread;
}

const std::string& Thread::GetName() {
    return t_thread_name;
}

void Thread::SetName(const std::string& name) {
    if(t_thread) {
        t_thread->m_name = name;
    }
    t_thread_name = name;
}

Thread::Thread(std::function<void()> cb, const std::string& name)
    :m_cb(cb)
    ,m_name(name) {
    if(name.empty()) {
        m_name = "UNKNOW";
    }
    int rt = pthread_create(&m_thread, nullptr, &Thread::run, this);
    if(rt) {
        CAPTAIN_LOG_ERROR(g_logger) << "pthread_create thread fail, rt=" << rt
            << " name=" << name;
        throw std::logic_error("pthread_create error");
    }

}

Thread::~Thread() {
    if(m_thread) {
        pthread_detach(m_thread); //detach
    }
}

void Thread::join() {
    if(m_thread) { //如果m_thread  说明该线程没结束，可以join
        int rt = pthread_join(m_thread, nullptr); //返回值为nullptr，不要返回值
        if(rt) {  //有问题  打印日志
            CAPTAIN_LOG_ERROR(g_logger) << "pthread_join thread fail, rt=" << rt
                << " name=" << m_name;
            throw std::logic_error("pthread_join error");
        }
        m_thread = 0;
    }
}

void* Thread::run(void* arg) {
    Thread* thread = (Thread*)arg; //转为线程参数
    t_thread = thread;
    t_thread_name = thread->m_name;
    thread->m_id = captain::GetThreadId();  //拿到线程id
    pthread_setname_np(pthread_self(), thread->m_name.substr(0, 15).c_str());

    std::function<void()> cb;
    //函数里有智能指针的时候，防止它的引用会出现在日志？不会被释放？
    cb.swap(thread->m_cb);

    //thread->m_semaphore.notify();

    cb();
    return 0;
}

}
