#include "../captain/include/captain.h"
#include <unistd.h>

captain::Logger::ptr g_logger = CAPTAIN_LOG_ROOT();

int count = 0;
//captain::RWMutex s_mutex;
captain::Mutex s_mutex;
void fun1() {
    CAPTAIN_LOG_INFO(g_logger) << "name: " << captain::Thread::GetName()
                             << " this.name: " << captain::Thread::GetThis()->getName()
                             << " id: " << captain::GetThreadId()
                             << " this.id: " << captain::Thread::GetThis()->getId();

    for(int i = 0; i < 100000; ++i) {
        //captain::RWMutex::WriteLock lock(s_mutex);
        captain::Mutex::Lock lock(s_mutex);
        ++count;
    }
}

void fun2() {
    while(true) {
        CAPTAIN_LOG_INFO(g_logger) << "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
    }
}

void fun3() {
    while(true) {
        CAPTAIN_LOG_INFO(g_logger) << "========================================";
    }
}

int main(int argc, char** argv) {
    CAPTAIN_LOG_INFO(g_logger) << "thread test begin";
    YAML::Node root = YAML::LoadFile("/home/lk/gitRepo/serverFramework/bin/conf/log2.yml");
    captain::Config::LoadFromYaml(root);
    std::vector<captain::Thread::ptr> thrs;  //线程池
    for(int i = 0; i < 2; ++i) {
        //captain::Thread::ptr thr(new captain::Thread(&fun1, "name_" + std::to_string(i)));
        captain::Thread::ptr thr(new captain::Thread(&fun2, "name_" + std::to_string(i * 2)));
        captain::Thread::ptr thr2(new captain::Thread(&fun3, "name_" + std::to_string(i * 2 + 1)));
        thrs.push_back(thr);
        thrs.push_back(thr2);
    }

    for(size_t i = 0; i < thrs.size(); ++i) {
        thrs[i]->join();
    }
    CAPTAIN_LOG_INFO(g_logger) << "thread test end";
    CAPTAIN_LOG_INFO(g_logger) << "count=" << count;

    return 0;
}
