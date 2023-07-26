#pragma once
#include <thread>
#include <functional>
#include <memory>
#include <pthread.h>

namespace captain {
class Thread {
public:
    typedef std::shared_ptr<Thread> ptr;
    Thread(std::function<void()> cb, const std::string& name);
    ~Thread();

    pid_t getId() const { return m_id;}
    const std::string& getName() const { return m_name;}

    void join();

    static Thread* GetThis(); //获得当前线程
    static const std::string& GetName();//获得当前线程名称  给日志用的
    static void SetName(const std::string& name); //更改线程名称 静态函数，在构造函数声明时就执行了
private:
    //禁止拷贝
    Thread(const Thread&) = delete;
    Thread(const Thread&&) = delete;
    Thread& operator=(const Thread&) = delete;

    static void* run(void* arg);
private:
    pid_t m_id = -1; //线程id
    pthread_t m_thread = 0;
    std::function<void()> m_cb;
    std::string m_name;  //线程名称

};

}