#include <iostream>
#include "../captain/include/log.h"
#include "../captain/include/util.h"

int main(int argc, char** argv) {
    captain::Logger::ptr logger(new captain::Logger);
    logger->addAppender(captain::LogAppender::ptr(new captain::StdoutLogAppender));

    captain::FileLogAppender::ptr file_appender(new captain::FileLogAppender("./log.txt"));
    captain::LogFormatter::ptr fmt(new captain::LogFormatter("%d%T%p%T%m%n"));
    file_appender->setFormatter(fmt);
    file_appender->setLevel(captain::LogLevel::ERROR);

    logger->addAppender(file_appender);

    //captain::LogEvent::ptr event(new captain::LogEvent(__FILE__, __LINE__, 0, captain::GetThreadId(), captain::GetFiberId(), time(0)));
    //captain::LogEvent::ptr event(new captain::LogEvent(__FILE__, __LINE__, 0,1,2,time(0)));
    //event->getSS() << "hello captain log";
    //logger->log(captain::LogLevel::DEBUG, event);
    std::cout << "hello captain log" << std::endl;

    CAPTAIN_LOG_INFO(logger) << "test macro";
    CAPTAIN_LOG_ERROR(logger) << "test macro error";

    CAPTAIN_LOG_FMT_ERROR(logger, "test macro fmt error %s", "aa");

    auto l = captain::LoggerMgr::GetInstance()->getLogger("xx");
    CAPTAIN_LOG_INFO(l) << "xxx";
    return 0;
}