#include "log.h"

#include <cassert>
#include <iostream>
#include <sstream>
#include <typeinfo>

#include "config.h"

namespace trycle
{

class PlainTextFormatItem : public LogFormatter::FormatItem
{
public:
    explicit PlainTextFormatItem(const std::string& str) : m_str(str) {}
    void format(std::ostream& out, LogEvent::ptr event) override
    {
        out << m_str;
    }

private:
    std::string m_str;
};

class LevelFormatItem : public LogFormatter::FormatItem
{
public:
    void format(std::ostream& out, LogEvent::ptr event) override
    {
        out << LogLevel::ToString(event->getLevel());
    }
};

class FilenameFormatItem : public LogFormatter::FormatItem
{
public:
    void format(std::ostream& out, LogEvent::ptr event)
    {
        out << event->getFilename();
    }
};

class LineFormatItem : public LogFormatter::FormatItem
{
public:
    void format(std::ostream& out, LogEvent::ptr event)
    {
        out << event->getLine();
    }
};

class ThreadIdFormatItem : public LogFormatter::FormatItem
{
public:
    void format(std::ostream& out, LogEvent::ptr event)
    {
        out << event->getThreadId();
    }
};

class FiberIdFormatItem : public LogFormatter::FormatItem
{
public:
    void format(std::ostream& out, LogEvent::ptr event)
    {
        out << event->getFiberId();
    }
};

class ContentFormatItem : public LogFormatter::FormatItem
{
public:
    void format(std::ostream& out, LogEvent::ptr event)
    {
        out << event->getContent();
    }
};

class TimeFormatItem : public LogFormatter::FormatItem
{
public:
    TimeFormatItem(const std::string& time_pattern = "%Y-%m-%dT%H:%M:%S")
        : m_time_pattern(time_pattern)
    {
    }

    void format(std::ostream& out, LogEvent::ptr event)
    {
        time_t time          = event->getTime();
        struct tm* time_info = localtime(&time);
        char buffer[64]{0};
        strftime(buffer, sizeof(buffer), m_time_pattern.c_str(), time_info);
        out << buffer;
    }

private:
    std::string m_time_pattern;
};

class NewLineFormatItem : public LogFormatter::FormatItem
{
public:
    void format(std::ostream& out, LogEvent::ptr event)
    {
        out << '\n';
    }
};

class TabFormatItem : public LogFormatter::FormatItem
{
public:
    void format(std::ostream& out, LogEvent::ptr event)
    {
        out << '\t';
    }
};

class PercentFormatItem : public LogFormatter::FormatItem
{
public:
    void format(std::ostream& out, LogEvent::ptr event)
    {
        out << '%';
    }
};

/**
 * %p 日志等级
 * %f 文件名
 * %l 行号
 * %d 日志时间
 * %t 线程号
 * %F 协程号
 * %m 日志消息
 * %n 换行
 * %% 百分号
 * %T 制表符
 */
thread_local static std::map<char, LogFormatter::FormatItem::ptr>
    format_item_map{
#define MAKE_FORMAT_ITEM(CH, ITEM) \
    {                              \
        CH, std::make_shared<ITEM>()}
        MAKE_FORMAT_ITEM('p', LevelFormatItem),
        MAKE_FORMAT_ITEM('f', FilenameFormatItem),
        MAKE_FORMAT_ITEM('l', LineFormatItem),
        MAKE_FORMAT_ITEM('d', TimeFormatItem),
        MAKE_FORMAT_ITEM('t', ThreadIdFormatItem),
        MAKE_FORMAT_ITEM('F', FiberIdFormatItem),
        MAKE_FORMAT_ITEM('m', ContentFormatItem),
        MAKE_FORMAT_ITEM('n', NewLineFormatItem),
        MAKE_FORMAT_ITEM('%', PercentFormatItem),
        MAKE_FORMAT_ITEM('T', TabFormatItem),
#undef MAKE_FORMAT_ITEM
    };

const std::string LogLevel::ToString(const LogLevel::Level& level)
{
    switch (level)
    {
#define LEVEL_TO_STRING(NAME) \
    case LogLevel::NAME:      \
        return #NAME;         \
        break;
        LEVEL_TO_STRING(DEBUG);
        LEVEL_TO_STRING(INFO);
        LEVEL_TO_STRING(WARN);
        LEVEL_TO_STRING(ERROR);
        LEVEL_TO_STRING(FATAL);
#undef LEVEL_TO_STRING
    }
    return "UNKNOWN";
}

const LogLevel::Level LogLevel::FromString(const std::string& str)
{
    std::string upper = str;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
#define LEVEL_FROM_STRING(LEVEL) \
    if (upper == #LEVEL)         \
    {                            \
        return LogLevel::LEVEL;  \
    }

    LEVEL_FROM_STRING(DEBUG);
    LEVEL_FROM_STRING(INFO);
    LEVEL_FROM_STRING(WARN);
    LEVEL_FROM_STRING(ERROR);
    LEVEL_FROM_STRING(FATAL);

    return LogLevel::UNKNOWN;
}

LogEvent::LogEvent(const std::string filename,
                   int32_t line,
                   uint32_t threadId,
                   uint32_t fiberId,
                   time_t time,
                   std::string content,
                   LogLevel::Level level)
    : m_filename(filename),
      m_line(line),
      //   m_elapse(elapse),
      m_threadId(threadId),
      m_fiberId(fiberId),
      m_time(time),
      m_content(content),
      m_level(level)
{
}

Logger::Logger(const std::string& name)
    : m_name(name),
      m_level(LogLevel::Level::DEBUG)
{
}

Logger::Logger(const std::string& name,
               LogLevel::Level level,
               LogFormatter::ptr log_formatter)
    : m_name(name),
      m_level(level),
      m_log_formatter(log_formatter)
{
}

void Logger::log(LogEvent::ptr event)
{
    MutexType::Lock lock(m_mutex);
    if (m_level <= event->getLevel())
    {
        if (!m_appenders.empty())
        {
            for (auto& it : m_appenders)
            {
                it->log(event->getLevel(), event);
            }
        }
        else if (m_root && m_root.get() != this)
        {
            m_root->log(event);
        }
    }
}

// void Logger::debug(LogEvent::ptr event)
// {
//     log(LogLevel::DEBUG, event);
// }
// void Logger::info(LogEvent::ptr event)
// {
//     log(LogLevel::INFO, event);
// }
// void Logger::warn(LogEvent::ptr event)
// {
//     log(LogLevel::WARN, event);
// }
// void Logger::error(LogEvent::ptr event)
// {
//     log(LogLevel::ERROR, event);
// }
// void Logger::fatal(LogEvent::ptr event)
// {
//     log(LogLevel::FATAL, event);
// }

void Logger::addAppender(LogAppender::ptr appender)
{
    MutexType::Lock lock(m_mutex);
    if (!appender->getFormatter())
    {
        MutexType::Lock alock(appender->m_mutex);
        appender->setFormatter(m_log_formatter);
    }

    m_appenders.push_back(appender);
}

void Logger::delAppender(LogAppender::ptr appender)
{
    MutexType::Lock lock(m_mutex);
    for (auto it = m_appenders.begin(), end = m_appenders.end(); it != end; ++it)
    {
        if (*it == appender)
        {
            m_appenders.erase(it);
        }
    }
}

void StdoutAppender::log(LogLevel::Level level, LogEvent::ptr event)
{
    if (m_level <= level)
    {
        MutexType::Lock lock(m_mutex);
        std::cout << m_formatter->format(event);
    }
}

FileAppender::FileAppender(const std::string& filename) : m_filename(filename)
{
    reopen();
}

void FileAppender::log(LogLevel::Level level, LogEvent::ptr event)
{
    if (m_level <= event->getLevel())
    {
        MutexType::Lock lock(m_mutex);
        m_filestream << m_formatter->format(event);
        m_filestream.flush();
    }
}

bool FileAppender::reopen()
{
    MutexType::Lock lock(m_mutex);
    if (!m_filestream)
    {
        m_filestream.close();
    }

    m_filestream.open(m_filename, std::ios_base::out | std::ios_base::app);
    return !!m_filestream;
}

LogFormatter::LogFormatter(const std::string& pattern) : m_pattern(pattern)
{
    init();
}

std::string LogFormatter::format(LogEvent::ptr event)
{
    std::stringstream str;
    for (auto it : m_format_item_list)
    {
        it->format(str, event);
    }
    // str << "\n";
    //  std::string ss = str.str();
    return str.str();
}

void LogFormatter::init()
{
    printf("LogFormatter initalizing...\n");
    enum PROC_STATUS
    {
        SCAN_STATUS,
        CREATE_STATUS,
    };
    PROC_STATUS proc_status = PROC_STATUS::SCAN_STATUS;
    int str_begin           = 0;
    size_t str_end          = 0;

    for (size_t i = 0, size = m_pattern.length(); i < size; i++)
    {
        MutexType::Lock lock(m_mutex);
        switch (proc_status)
        {
            case SCAN_STATUS:
                str_begin = i;
                for (str_end = str_begin; str_end < size; str_end++)
                {
                    if (m_pattern[str_end] == '%')
                    {
                        proc_status = CREATE_STATUS;
                        break;
                    }
                    // not found
                }

                i = str_end;
                if (str_begin == str_end)
                {
                    continue;
                }

                m_format_item_list.push_back(std::make_shared<PlainTextFormatItem>(
                    m_pattern.substr(str_begin, str_end - str_begin)));

                break;
            case CREATE_STATUS:
                assert(!format_item_map.empty() && "format_item_map must to be initialized first.");
                auto find_itor = format_item_map.find(m_pattern[i]);
                if (find_itor == format_item_map.end())
                {
                    m_format_item_list.push_back(std::make_shared<PlainTextFormatItem>("<error_format>"));
                }
                else
                {

                    m_format_item_list.push_back(find_itor->second);
                }

                proc_status = SCAN_STATUS;
                break;
        }
    }
    printf("LogFormatter initalized...\n");
}

__LoggerManager::__LoggerManager()
{
    // init();
}

Logger::ptr makeLogger(const LogConfig& config)
{
    auto formatter = std::make_shared<LogFormatter>(config.formatter);
    auto logger    = std::make_shared<Logger>(config.name, config.level, formatter);

    for (const auto& item : config.appenders)
    {
        auto level = item.level;
        if (level == LogLevel::Level::UNKNOWN)
        {
            level = config.level;
        }

        LogAppender::ptr appender{nullptr};
        if (item.type == 1)
        {
            appender = std::make_shared<StdoutAppender>();
        }
        else if (item.type == 2)
        {
            appender = std::make_shared<FileAppender>(item.file);
        }

        appender->setLevel(level);
        appender->setFormatter(formatter);
        logger->addAppender(appender);
    }

    return logger;
}

void __LoggerManager::init(const std::set<LogConfig>& log_configs)
{
    MutexType::Lock lock(m_mutex);
    // auto log_configs = Config::lookUp("logs", std::set<LogConfig>());
    // Config::loadFromYAML(YAML::LoadFile("../conf/config.yaml"));

    // log_configs->add_listener(100, [](const LogConfig& old_val, const LogConfig& new_val)
    //                           { std::cout << "OnChange | old_val=" << old_val.name << ", new_val=" << new_val.name << "\n"; });
    // for (const auto& log_config : log_configs->getVal())
    for (const auto& log_config : log_configs)
    {
        auto logger = makeLogger(log_config);
        // m_logger_map.erase(logger->m_name);
        m_logger_map[logger->m_name] = logger;

        if (logger->m_name == "root")
        {
            m_root = logger;
        }
    }
}

// void __LoggerManager::init()
// {
//     // static
//     auto log_formater = std::make_shared<LogFormatter>("%d [%p] %f:%l [%t-%F] - %m%n");
//     auto root         = std::make_shared<Logger>("root", LogLevel::Level::DEBUG, log_formater);
//     m_root            = root;
//     m_root->addAppender(std::make_shared<StdoutAppender>());
//     m_root->addAppender(std::make_shared<FileAppender>("./log.txt"));
//     m_logger_map.insert({"root", root});

//     // const Logger::ptr logger = std::make_shared<Logger>(logger_name, m_root->getLevel(), m_root->getLogFormater());
//     // logger->setLogAppenders(m_root->getLogAppenders());

//     const std::string logger_name{"adf"};
//     // m_logger_map.insert(std::make_pair<std::string, Logger::ptr>(logger_name, logger));
//     Logger::ptr logger;
//     m_logger_map.insert({logger_name, logger});

//     Logger::ptr loggerx;
//     m_logger_map.insert({logger_name, loggerx});
// }

Logger::ptr __LoggerManager::getLogger(const std::string logger_name)
{
    auto itor = m_logger_map.find(logger_name);
    if (itor != m_logger_map.end())
    {
        return itor->second;
    }

    const Logger::ptr logger = std::make_shared<Logger>(logger_name);
    logger->m_root           = m_root;

    m_logger_map.insert({logger_name, logger});

    return logger;
}

} // namespace trycle