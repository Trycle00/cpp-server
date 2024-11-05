#include <sstream>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <typeinfo>
#include <yaml-cpp/yaml.h>

#include "config.h"
#include "log.h"
#include "util.h"

trycle::ConfigVar<int>::ptr int_val = trycle::Config::lookUp("test.int_val", 1);
auto int_vec                        = trycle::Config::lookUp("test.int_vec", std::vector<int>{1, 2, 3}, "");
auto int_list                       = trycle ::Config::lookUp("test.int_list", std::list<int>{10, 20, 30});
auto int_set                        = trycle ::Config::lookUp("test.int_set", std::set<int>{100, 200, 300});
auto int_map                        = trycle ::Config::lookUp("test.int_map", std::map<std::string, int>{{"k1", 100}, {"k2", 200}});

void TEST_loadConfig(const std::string& file)
{
    LOG_ERROR(GET_ROOT_LOGGER, "Loading yaml config ...");
    YAML::Node root;
    try
    {
        root = YAML::LoadFile(file);
    }
    catch (const std::exception& e)
    {
        LOG_FMT_ERROR(GET_ROOT_LOGGER, "Load yaml file error | %s ", e.what())
    }
    trycle::Config::loadFromYAML(root);
}

int main(int argc, char** argv)
{
    printf("1======================\n");
    printf("TEST_config:\n");

    // // trycle::Config::lookUp<std::string>("system.port", "11", "");
    // LOG_FMT_DEBUG(GET_ROOT_LOGGER, "Before: %s", trycle::Config::lookUp<std::string>("system.port", "11")->toString().c_str());
    // LOG_FMT_DEBUG(GET_ROOT_LOGGER, "Before: %s", trycle::Config::lookUp<std::string>("system.name", "22")->toString().c_str());
    // // YAML::Node root2 = YAML::LoadFile("./conf/config.yaml");
    // // trycle::Config::loadFromYAML(root2);
    // TEST_loadConfig("./conf/config.yaml");

    // LOG_FMT_DEBUG(GET_ROOT_LOGGER, "After: %s", trycle::Config::lookUp("system.port")->toString().c_str());
    // LOG_FMT_DEBUG(GET_ROOT_LOGGER, "After: %s", trycle::Config::lookUp("system.name")->toString().c_str());

    printf("----------------------\n");

#define PRINT_C(config_var, prefix)                                                                                               \
    {                                                                                                                             \
        for (auto& i : config_var->getVal())                                                                                      \
        {                                                                                                                         \
            LOG_FMT_DEBUG(GET_ROOT_LOGGER, "" #prefix " | %s=%s", config_var->get_var_name().c_str(), std::to_string(i).c_str()); \
        }                                                                                                                         \
    }
    PRINT_C(int_vec, "Before");
    PRINT_C(int_list, "Before");
    PRINT_C(int_set, "Before");

#define PRINT_M(config_var, prefix)                                                                                     \
    {                                                                                                                   \
        for (auto& pair : config_var->getVal())                                                                         \
        {                                                                                                               \
            LOG_FMT_DEBUG(GET_ROOT_LOGGER, "" #prefix " | %s : %s=%s",                                                  \
                          config_var->get_var_name().c_str(), pair.first.c_str(), std::to_string(pair.second).c_str()); \
        }                                                                                                               \
    }

    PRINT_C(int_vec, "Before");
    PRINT_C(int_list, "Before");
    PRINT_C(int_set, "Before");
    PRINT_M(int_map, "Before");

    printf("----------------------\n");

    TEST_loadConfig("../conf/config.yaml");
    printf("----------------------\n");

    PRINT_C(int_vec, "After");
    PRINT_C(int_list, "After");
    PRINT_C(int_set, "After");
    PRINT_M(int_map, "After");

    printf("----------------------\n");

    return 0;
}