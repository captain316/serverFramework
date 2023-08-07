#include "captain/include/captain.h"

static captain::Logger::ptr g_logger = CAPTAIN_LOG_ROOT();

void test_fiber() {
    static int s_count = 5;
    CAPTAIN_LOG_INFO(g_logger) << "test in fiber s_count=" << s_count;

    sleep(1);
    if(--s_count >= 0) {
        //captain::Scheduler::GetThis()->schedule(&test_fiber);
        captain::Scheduler::GetThis()->schedule(&test_fiber, captain::GetThreadId());
    }
}

int main(int argc, char** argv) {
    CAPTAIN_LOG_INFO(g_logger) << "main";
    captain::Scheduler sc(3, false, "test");
    sc.start();
    sleep(2);
    CAPTAIN_LOG_INFO(g_logger) << "schedule";
    sc.schedule(&test_fiber);
    sc.stop();
    CAPTAIN_LOG_INFO(g_logger) << "over";
    return 0;

}
