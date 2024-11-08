
#ifndef TRY_LOGGER_H
#define TRY_LOGGER_H

#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <memory>

#include <boost/lexical_cast.hpp>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>

#include "singleton.h"
#include "thread.h"
#include "util.h"
// #include "config.h"

#define MAKE_LOG_EVENT(level, message) \
    std::make_shared<trycle::LogEvent>(__FILE__, __LINE__, trycle::GetThreadId(), trycle::GetFiberId(), time(0), message, level)

#define LOG_LEVEL(logger, level, message) \
    logger->log(MAKE_LOG_EVENT(level, message))

#define LOG_DEBUG(logger, message) LOG_LEVEL(logger, trycle::LogLevel::DEBUG, message)
#define LOG_INFO(logger, message) LOG_LEVEL(logger, trycle::LogLevel::INFO, message)
#define LOG_WARN(logger, message) LOG_LEVEL(logger, trycle::LogLevel::WARN, message)
#define LOG_ERROR(logger, message) LOG_LEVEL(logger, trycle::LogLevel::ERROR, message)
#define LOG_FATAL(logger, message) LOG_LEVEL(logger, trycle::LogLevel::FATAL, message)

#define LOG_FMT_LEVEL(logger, level, format, argv...)     \
    {                                                     \
        char* dyn_buf = nullptr;                          \
        int written   = asprintf(&dyn_buf, format, argv); \
        if (written != -1)                                \
        {                                                 \
            LOG_LEVEL(logger, level, dyn_buf);            \
            free(dyn_buf);                                \
        }                                                 \
    }

#define LOG_FMT_DEBUG(logger, format, argv...) LOG_FMT_LEVEL(logger, trycle::LogLevel::DEBUG, format, argv)
#define LOG_FMT_INFO(logger, format, argv...) LOG_FMT_LEVEL(logger, trycle::LogLevel::INFO, format, argv)
#define LOG_FMT_WARN(logger, format, argv...) LOG_FMT_LEVEL(logger, trycle::LogLevel::WARN, format, argv)
#define LOG_FMT_ERROR(logger, format, argv...) LOG_FMT_LEVEL(logger, trycle::LogLevel::ERROR, format, argv)
#define LOG_FMT_FATAL(logger, format, argv...) LOG_FMT_LEVEL(logger, trycle::LogLevel::FATAL, format, argv)

#define LOG_GET(name) trycle::LoggerManager::GetSingleton()->getLogger(name)

#define GET_ROOT_LOGGER trycle::LoggerManager::GetSingleton()->getRoot()
#define GET_LOGGER(name) trycle::LoggerManager::GetSingleton()->getLogger(name)

namespace trycle
{

// class Mutex;

// 日志级别
class LogLevel
{
public:
    typedef std::shared_ptr<LogLevel> ptr;

    enum Level
    {
        UNKNOWN = 0,
        DEBUG   = 1,
        INFO    = 2,
        WARN    = 3,
        ERROR   = 4,
        FATAL   = 5,
    };

    static const std::string ToString(const LogLevel::Level& level);
    static const LogLevel::Level FromString(const std::string& level);
};

struct LogAppenderConfig
{
    typedef std::shared_ptr<LogAppenderConfig> ptr;
    int type;
    LogLevel::Level level = LogLevel::Level::UNKNOWN;
    std::string file;
    bool operator==(const LogAppenderConfig& right) const
    {
        return level == right.level && type == right.type && file == right.file;
    }
    bool operator<(const LogAppenderConfig& right) const
    {
        return type < right.type;
    }

    friend std::ostream& operator<<(std::ostream& out, const LogAppenderConfig& operand)
    {
        out << operand.type;
        return out;
    }
};

struct LogConfig
{
    typedef std::shared_ptr<LogConfig> ptr;
    LogLevel::Level level = LogLevel::Level::UNKNOWN;
    std::string name;
    std::string formatter;
    std::set<LogAppenderConfig> appenders;

    bool operator==(const LogConfig& right) const
    {
        // return level == right.level && name == right.name && formatter == right.formatter && appenders == right.appenders;
        return name == right.name;
    }
    bool operator<(const LogConfig& right) const
    {
        return level < right.level;
    }

    friend std::ostream& operator<<(std::ostream& out, const LogConfig& operand)
    {
        out << operand.name;
        return out;
    }
};

// 日志事件
class LogEvent
{
public:
    typedef std::shared_ptr<LogEvent> ptr;
    LogEvent(
        const std::string m_filename,
        int32_t m_line,
        uint32_t m_threadId,
        uint32_t m_fiberId,
        time_t m_time,
        std::string m_content,
        LogLevel::Level level);

    LogLevel::Level getLevel() const { return m_level; }
    std::string& getFilename() { return m_filename; }
    int32_t getLine() const { return m_line; }
    // uint32_t getElapse() const { return m_elapse; }
    uint64_t getThreadId() const { return m_threadId; }
    uint64_t getFiberId() const { return m_fiberId; }
    time_t getTime() const { return m_time; }
    std::string getContent() const { return m_content; }

private:
    LogLevel::Level m_level; // 日志等级
    std::string m_filename;  // 文件名
    int32_t m_line = 0;      // 行号
    // uint32_t m_elapse   = 0; // 从启动到现在的毫秒数
    uint64_t m_threadId = 0; // 线程id
    uint64_t m_fiberId  = 0; // 协程id
    time_t m_time       = 0; // 时间
    std::string m_content;   // 内容
};

// 日志格式器
class LogFormatter
{
    friend class Logger;

public:
    typedef std::shared_ptr<LogFormatter> ptr;

    class FormatItem
    {
    public:
        typedef std::shared_ptr<FormatItem> ptr;
        virtual void format(std::ostream& out, LogEvent::ptr event) = 0;
    };

    explicit LogFormatter(const std::string& pattern);
    std::string format(LogEvent::ptr event);

private:
    void init();

    std::string m_pattern;                           // 日志格式
    std::vector<FormatItem::ptr> m_format_item_list; // 格式项列表
    Mutex m_mutex;
};

// 日志输出器
class LogAppender
{
    friend class Logger;

public:
    typedef std::shared_ptr<LogAppender> ptr;
    virtual ~LogAppender() {}

    virtual void log(LogLevel::Level level, LogEvent::ptr event) = 0;

    void setLevel(LogLevel::Level level) { m_level = level; }
    LogLevel::Level getLevel() const { return m_level; }

    void setFormatter(LogFormatter::ptr formatter) { m_formatter = formatter; }
    LogFormatter::ptr getFormatter() { return m_formatter; }

protected:
    LogLevel::Level m_level;
    LogFormatter::ptr m_formatter;
    Mutex m_mutex;
};

// 日志器
class Logger
{
    friend class __LoggerManager;

public:
    typedef std::shared_ptr<Logger> ptr;
    Logger(const std::string& name);
    Logger(const std::string& name, LogLevel::Level level, LogFormatter::ptr log_formatter);
    void log(LogEvent::ptr event);
    // void log(LogLevel::Level level, LogEvent::ptr event);

    void debug(LogEvent::ptr event);
    void info(LogEvent::ptr event);
    void warn(LogEvent::ptr event);
    void error(LogEvent::ptr event);
    void fatal(LogEvent::ptr event);

    void addAppender(LogAppender::ptr appender);
    void delAppender(LogAppender::ptr appender);

    void setName(const std::string name)
    {
        m_name = name;
    }

    LogLevel::Level getLevel()
    {
        return m_level;
    }
    void setLevel(LogLevel::Level level)
    {
        m_level = level;
    }

    LogFormatter::ptr getLogFormater() const
    {
        return m_log_formatter;
    }

    void setLogFormater(LogFormatter::ptr log_formater)
    {
        m_log_formatter = log_formater;
    }

    std::list<LogAppender::ptr> getLogAppenders() const
    {
        return m_appenders;
    }

    void setLogAppenders(std::list<LogAppender::ptr> appenders)
    {
        m_appenders = appenders;
    }

private:
    std::string m_name;                        // 日志名称
    LogLevel::Level m_level = LogLevel::DEBUG; // 日志级别
    LogFormatter::ptr m_log_formatter;
    std::list<LogAppender::ptr> m_appenders; // Appender集合
    Logger::ptr m_root;                      // root logger use as default
    Mutex m_mutex;
};

// 定义输出到控制台的Appender
class StdoutAppender : public LogAppender
{
public:
    typedef std::shared_ptr<StdoutAppender> ptr;

    void log(LogLevel::Level level, LogEvent::ptr event) override;

private:
};

// 定义输出到文件的Appender
class FileAppender : public LogAppender
{
public:
    typedef std::shared_ptr<FileAppender> ptr;

    FileAppender(const std::string& fileName);

    void log(LogLevel::Level level, LogEvent::ptr event) override;

    // 重新打开文件，如果打开成功返回true
    bool reopen();

private:
    std::string m_filename;
    std::fstream m_filestream;
    Mutex m_mutex;
};

// 日志管理类
class __LoggerManager
{
public:
    typedef std::shared_ptr<__LoggerManager> ptr;
    __LoggerManager();

    Logger::ptr getLogger(const std::string logger_name);

    Logger::ptr getRoot()
    {
        return m_root;
    }

    void init(const std::set<LogConfig>& log_configs);

private:
    std::map<std::string, Logger::ptr> m_logger_map;
    Logger::ptr m_root;
    Mutex m_mutex;
};

typedef SingletonPtr<__LoggerManager> LoggerManager;

} // namespace trycle

#endif // TRY_LOGGER_H