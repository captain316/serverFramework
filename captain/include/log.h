#pragma once

#include <string>
#include <stdint.h>
#include <memory>
#include <list>
#include <sstream>
#include <iostream>
#include <fstream>
#include <vector>
#include <stdarg.h>
#include <map>
#include "util.h"
#include "singleton.h"
#include "thread.h"
//CAPTAIN_LOG_LEVEL(logger, level) 宏用于判断当前日志级别是否满足打印条件，
//并通过 captain::LogEventWrap 创建一个 captain::LogEvent 对象，
//并最终通过 .getSS() 获取一个用于日志输出的 std::stringstream 对象，将日志消息输出。
#define CAPTAIN_LOG_LEVEL(logger, level) \
    if(logger->getLevel() <= level) \
        captain::LogEventWrap(captain::LogEvent::ptr(new captain::LogEvent(logger, level, \
                        __FILE__, __LINE__, 0, captain::GetThreadId(),\
                captain::GetFiberId(), time(0), captain::Thread::GetName()))).getSS()

#define CAPTAIN_LOG_DEBUG(logger) CAPTAIN_LOG_LEVEL(logger, captain::LogLevel::DEBUG)
#define CAPTAIN_LOG_INFO(logger) CAPTAIN_LOG_LEVEL(logger, captain::LogLevel::INFO)
#define CAPTAIN_LOG_WARN(logger) CAPTAIN_LOG_LEVEL(logger, captain::LogLevel::WARN)
#define CAPTAIN_LOG_ERROR(logger) CAPTAIN_LOG_LEVEL(logger, captain::LogLevel::ERROR)
#define CAPTAIN_LOG_FATAL(logger) CAPTAIN_LOG_LEVEL(logger, captain::LogLevel::FATAL)

#define CAPTAIN_LOG_FMT_LEVEL(logger, level, fmt, ...) \
    if(logger->getLevel() <= level) \
        captain::LogEventWrap(captain::LogEvent::ptr(new captain::LogEvent(logger, level, \
                        __FILE__, __LINE__, 0, captain::GetThreadId(),\
                captain::GetFiberId(), time(0), captain::Thread::GetName()))).getEvent()->format(fmt, __VA_ARGS__)

#define CAPTAIN_LOG_FMT_DEBUG(logger, fmt, ...) CAPTAIN_LOG_FMT_LEVEL(logger, captain::LogLevel::DEBUG, fmt, __VA_ARGS__)
#define CAPTAIN_LOG_FMT_INFO(logger, fmt, ...)  CAPTAIN_LOG_FMT_LEVEL(logger, captain::LogLevel::INFO, fmt, __VA_ARGS__)
#define CAPTAIN_LOG_FMT_WARN(logger, fmt, ...)  CAPTAIN_LOG_FMT_LEVEL(logger, captain::LogLevel::WARN, fmt, __VA_ARGS__)
#define CAPTAIN_LOG_FMT_ERROR(logger, fmt, ...) CAPTAIN_LOG_FMT_LEVEL(logger, captain::LogLevel::ERROR, fmt, __VA_ARGS__)
#define CAPTAIN_LOG_FMT_FATAL(logger, fmt, ...) CAPTAIN_LOG_FMT_LEVEL(logger, captain::LogLevel::FATAL, fmt, __VA_ARGS__)

#define CAPTAIN_LOG_ROOT() captain::LoggerMgr::GetInstance()->getRoot()
#define CAPTAIN_LOG_NAME(name) captain::LoggerMgr::GetInstance()->getLogger(name)

namespace captain {

class Logger;
class LoggerManager;

//日志级别
class LogLevel{
public:
    enum Level{
        UNKNOW = 0,
        DEBUG = 1,
        INFO = 2,
        WARN = 3,
        ERROR = 4,
        FATAL = 5
    };

    static const char* ToString(LogLevel::Level level);
    static LogLevel::Level FromString(const std::string& str);
};

//日志事件
class LogEvent{
public:
    typedef std::shared_ptr<LogEvent> ptr;
    LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level, const char* file, int32_t m_line, uint32_t elapse,
            uint32_t thread_id, uint32_t fiber_id, uint64_t time
            ,const std::string& thread_name);

    const char* getFile() const { return m_file;}
    int32_t getLine() const { return m_line;}
    uint32_t getElapse() const { return m_elapse;}
    uint32_t getThreadId() const { return m_threadId;}
    uint32_t getFiberId() const { return m_fiberId;}
    uint64_t getTime() const { return m_time;}
    const std::string& getThreadName() const { return m_threadName;}
    std::string getContent() const { return m_ss.str();}
    std::shared_ptr<Logger> getLogger() const { return m_logger; }
    LogLevel::Level getLevel() const { return m_level; }

    std::stringstream& getSS() { return m_ss;}
    void format(const char* fmt, ...);
    void format(const char* fmt, va_list al);
private:
    const char* m_file = nullptr; //文件名
    int32_t m_line = 0;           //行号
    uint32_t m_elapse = 0;        //程序启动开始到现在的毫秒数
    uint32_t m_threadId =0;       //线程id
    uint32_t m_fiberId = 0;       //协程id 
    uint64_t m_time;             //时间戳
    std::string m_threadName;
    std::stringstream m_ss;

    std::shared_ptr<Logger> m_logger;
    LogLevel::Level m_level;
};

class LogEventWrap {
public:
    LogEventWrap(LogEvent::ptr e);
    ~LogEventWrap();
    LogEvent::ptr getEvent() const { return m_event;}
    std::stringstream& getSS();
private:
    LogEvent::ptr m_event;
};

//日志格式器
class LogFormatter {
public:
    typedef std::shared_ptr<LogFormatter> ptr;
    LogFormatter(const std::string& pattern);

    //%t    %thread_id %m%n
    std::string format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);
public:
    class FormatItem{
    public:
        typedef std::shared_ptr<FormatItem> ptr;
        virtual ~FormatItem(){}
        virtual void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;
    };

    void init();
    bool isError() const { return m_error;}
    const std::string getPattern() const { return m_pattern;}
private:
    std::string m_pattern;
    std::vector<FormatItem::ptr> m_items;
    bool m_error = false;
};

//日志输出地
class LogAppender{
friend class Logger;
public:
    typedef std::shared_ptr<LogAppender> ptr;
    //MutexType是为了方便测试不同的锁，如真锁Mutex和假锁NullRWMutex、NullMutex
    //typedef Mutex MutexType;  //Mutex性能低
    //typedef NullMutex MutexType;  //测试假锁 
    typedef Spinlock MutexType;  //Spinlock性能高，但cpu占用也高。
    //typedef CASLock MutexType;
    virtual ~LogAppender(){}

    virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;
    virtual std::string toYamlString() = 0;  //用到了format  需要加锁。

    void setFormatter (LogFormatter::ptr val);
    LogFormatter::ptr getFormatter();

    LogLevel::Level getLevel() const { return m_level;} //基础/原子类型 可加可不加锁
    void setLevel(LogLevel::Level val) { m_level = val;}
protected:
    LogLevel::Level m_level = LogLevel::DEBUG;
    bool m_hasFormatter = false;
    MutexType m_mutex;   //锁
    LogFormatter::ptr m_formatter;
};

//日志器
class Logger : public std::enable_shared_from_this<Logger> {
friend class LoggerManager;
public:
    typedef std::shared_ptr<Logger> ptr;
    //MutexType是为了方便测试不同的锁，如真锁Mutex和假锁NullRWMutex、NullMutex
    //typedef Mutex MutexType;  //Mutex性能低
    //typedef NullMutex MutexType;  //测试假锁
    typedef Spinlock MutexType;  //Spinlock性能高，但cpu占用也高。
    //typedef CASLock MutexType;
    Logger(const std::string& name = "root");
    void log(LogLevel::Level level, LogEvent::ptr event);

    void debug(LogEvent::ptr event);
    void info(LogEvent::ptr event);
    void warn(LogEvent::ptr event);
    void error(LogEvent::ptr event);
    void fatal(LogEvent::ptr event);

    void addAppender(LogAppender::ptr appender);
    void delAppender(LogAppender::ptr appender);
    void clearAppenders();
    LogLevel::Level getLevel() const { return m_level; }
    void setLevel(LogLevel::Level val) { m_level = val; }

    const std::string& getName() const { return m_name;}
    void setFormatter(LogFormatter::ptr val);
    void setFormatter(const std::string& val);
    LogFormatter::ptr getFormatter();

    std::string toYamlString();
private:
    std::string m_name;     //日志名称
    LogLevel::Level m_level;//日志级别
    MutexType m_mutex;
    std::list<LogAppender::ptr> m_appenders; //日志集合
    LogFormatter::ptr m_formatter;
    Logger::ptr m_root;
};

//输出到控制台的Appender
class StdoutLogAppender : public LogAppender {
public:
    typedef std::shared_ptr<StdoutLogAppender> ptr;
    void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;
    std::string toYamlString() override;
};

//输出到文件的Appender
class FileLogAppender : public LogAppender {
public:
    typedef std::shared_ptr<FileLogAppender> ptr;
    FileLogAppender(const std::string& filename);
    void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;
    std::string toYamlString() override;

    //重新打开文件  成功打开文件return ture
    bool reopen();
private:
    std::string m_filename;
    std::ofstream m_filestream;
    uint64_t m_lastTime = 0;  //用来检查输出文件是否被删除的一个信号  防止内存没释放 资源被占用
};

class LoggerManager {
public:
    //MutexType是为了方便测试不同的锁，如真锁Mutex和假锁NullRWMutex、NullMutex
    //typedef Mutex MutexType;  //Mutex性能低。
    //typedef NullMutex MutexType;  //测试假锁。输出日志可能会串行
    typedef Spinlock MutexType;  //Spinlock性能高，但cpu占用也高。
    //typedef CASLock MutexType;
    LoggerManager();
    Logger::ptr getLogger(const std::string& name);

    void init();
    Logger::ptr getRoot() const { return m_root;}

    std::string toYamlString();
private:
    MutexType m_mutex;
    std::map<std::string, Logger::ptr> m_loggers;
    Logger::ptr m_root;
};

typedef captain::Singleton<LoggerManager> LoggerMgr;

}