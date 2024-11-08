#ifndef TRY_INITIALIZE_H
#define TRY_INITIALIZE_H

#include "config.h"
#include "log.h"

namespace trycle
{

void initialize()
{

    printf("init logger by yaml config...\n");
    auto log_configs = trycle::Config::lookUp("logs", std::set<trycle::LogConfig>());
    trycle::Config::loadFromYAML(YAML::LoadFile("../conf/config.yaml"));

    log_configs->add_listener(100, [](const std::set<trycle::LogConfig>& old_val, const std::set<trycle::LogConfig>& new_val)
                              {
                                  printf("onchange LogConfig...");
                                  trycle::LoggerManager::GetSingleton()->init(new_val); });

    trycle::LoggerManager::GetSingleton()->init(log_configs->getVal());
}

} // namespace trycle

#endif // TRY_INITIALIZE_H