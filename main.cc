#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <typeinfo>

#include "config.h"
#include "log.h"
#include "util.h"

int main(int argc, char** argv)
{
    printf("1======================\n");
    printf("TEST_defaultLogger:\n");
    // // LogFormatter log_formatter("%p %s %t asdf %% %l %f");
    // auto logger = std::make_shared<Logger>("root", LogLevel::INFO, std::make_shared<LogFormatter>("%d [%p] %f:%l [%t-%F] - %m%n"));

    // logger->addAppender(std::make_shared<StdoutAppender>());
    // logger->addAppender(std::make_shared<FileAppender>("FileAppender"));

    // logger.info(std::make_shared<LogEvent>(__FILE__, __LINE__,  1, 2, time(0), "Hello..world"));

    printf("2----------------------\n");
    printf("TEST_macroLogger:\n");

    auto logger = std::make_shared<trycle::Logger>("root", trycle::LogLevel::DEBUG, std::make_shared<trycle::LogFormatter>("%d [%p] %f:%l [%t-%F] - %m%n"));

    logger->addAppender(std::make_shared<trycle::StdoutAppender>());
    logger->addAppender(std::make_shared<trycle::FileAppender>("FileAppender"));
    auto e = MAKE_LOG_EVENT(trycle::LogLevel::Level::DEBUG, "Hello 222222222222");
    // logger->log( MAKE_LOG_EVENT(trycle::LogLevel::Level::DEBUG, "Hello 222222222222") );

    LOG_LEVEL(logger, trycle::LogLevel::Level::DEBUG, "Hello 222222222222");
    LOG_DEBUG(logger, "Hello 3333333333333");
    LOG_INFO(logger, "Hello 44444444444");
    LOG_WARN(logger, "Hello 555555555555");
    LOG_ERROR(logger, "Hello 6666666666");
    LOG_FATAL(logger, "Hello 777777777777");
    LOG_FATAL(logger, "Hello 777777777777");
    LOG_FATAL(logger, "Hello threadId=" + std::to_string(trycle::GetThreadId()));

    printf("----------------------\n");
    printf("TEST_LOG_FMT_LOGGER:\n");
    LOG_FMT_ERROR(GET_ROOT_LOGGER, "ConfigVar::toString Exception | %s | convert %s to string.",
                  "a1111", "b22222");

    printf("----------------------\n");
    printf("TEST_loggerManager:\n");

    // trycle::LoggerManager->
    auto test = trycle::LoggerManager::GetSingleton()->getLogger("test");
    // trycle::LoggerManager->
    LOG_DEBUG(test, "trycle::LoggerManager-> Hello 11111111111");

    printf("----------------------\n");
    printf("TEST_Config:\n");

    trycle::ConfigVar<int>::ptr var1 = trycle::Config::lookUp("test.k.var1", (int)111, "var1 is 111");
    LOG_FMT_DEBUG(GET_ROOT_LOGGER, "%s=%s", var1->get_var_name().c_str(), var1->toString().c_str());

    trycle::ConfigVar<std::string>::ptr var2 = trycle::Config::lookUp("test.k.var2", std::string("aaa"), "var2 is aaa");
    LOG_FMT_DEBUG(GET_ROOT_LOGGER, "%s=%s", var2->get_var_name().c_str(), var2->toString().c_str());


    printf("----------------------\n");

    return 0;
}