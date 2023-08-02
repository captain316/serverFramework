#include "captain/include/captain.h"

captain::Logger::ptr g_logger = CAPTAIN_LOG_ROOT();

void run_in_fiber() {
    CAPTAIN_LOG_INFO(g_logger) << "run_in_fiber begin";
    captain::Fiber::YieldToHold();
    CAPTAIN_LOG_INFO(g_logger) << "run_in_fiber end";
    captain::Fiber::YieldToHold();
}

/* void test_fiber() {
    CAPTAIN_LOG_INFO(g_logger) << "main begin -1";
    {
        captain::Fiber::GetThis();
        CAPTAIN_LOG_INFO(g_logger) << "main begin";
        captain::Fiber::ptr fiber(new captain::Fiber(run_in_fiber));
        fiber->swapIn();
        CAPTAIN_LOG_INFO(g_logger) << "main after swapIn";
        fiber->swapIn();
        CAPTAIN_LOG_INFO(g_logger) << "main after end";
        fiber->swapIn();
    }
    CAPTAIN_LOG_INFO(g_logger) << "main after end2";
} */

int main(int argc, char** argv) {
    captain::Fiber::GetThis();
    CAPTAIN_LOG_INFO(g_logger) << "main begin";
    captain::Fiber::ptr fiber(new captain::Fiber(run_in_fiber));
    fiber->swapIn();
    CAPTAIN_LOG_INFO(g_logger) << "main after swapIn";
    fiber->swapIn();
    CAPTAIN_LOG_INFO(g_logger) << "main after end";
    //fiber->swapIn();


/*     captain::Thread::SetName("main");

    std::vector<captain::Thread::ptr> thrs;
    for(int i = 0; i < 3; ++i) {
        thrs.push_back(captain::Thread::ptr(
                    new captain::Thread(&test_fiber, "name_" + std::to_string(i))));
    }
    for(auto i : thrs) {
        i->join();
    } */
    return 0;
}
