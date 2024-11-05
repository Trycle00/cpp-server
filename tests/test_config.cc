#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <typeinfo>
#include <yaml-cpp/yaml.h>
#include <sstream>

#include "config.h"
#include "log.h"
#include "util.h"


trycle::ConfigVar<int>::ptr int_val = trycle::Config::lookUp("test.int_val", 1);
// auto int_vec = trycle::Config::lookUp("test.int_vec", std::vector<int>{1,2,3}, "");


int main(int argc, char** argv)
{
    printf("1======================\n");
    // printf("TEST_config:\n");

    // // trycle::Config::lookUp<std::string>("system.port", "11", "");
    // LOG_FMT_DEBUG(GET_ROOT_LOGGER, "Before: %s", trycle::Config::lookUp<std::string>("system.port", "11")->toString().c_str());
    // LOG_FMT_DEBUG(GET_ROOT_LOGGER, "Before: %s", trycle::Config::lookUp<std::string>("system.name", "22")->toString().c_str());
    // YAML::Node root2 = YAML::LoadFile("conf/config.yaml");
    // trycle::Config::loadFromYAML(root2);

    // LOG_FMT_DEBUG(GET_ROOT_LOGGER, "After: %s", trycle::Config::lookUp("system.port")->toString().c_str());
    // LOG_FMT_DEBUG(GET_ROOT_LOGGER, "After: %s", trycle::Config::lookUp("system.name")->toString().c_str());

    // printf("----------------------\n");

    // printf("TEST_LexicalCast:\n");

    // std::string ss = trycle::LexicalCast<int, std::string>()(1);
    // LOG_FMT_DEBUG(GET_ROOT_LOGGER, "trycle::LexicalCast<int, std::string>()(1) = %s", ss.c_str());
    // std::vector<YAML::Node> nodes{root2};
    // const std::string nodes_to_str = trycle::LexicalCast<std::vector<YAML::Node>, std::string>()(nodes);
    // LOG_FMT_DEBUG(GET_ROOT_LOGGER, "nodes_to_str = %s", nodes_to_str.c_str());

    // printf("------------\n");
    // std::cout << std::endl;
    // std::cout << root2 << "\n";

    // printf("------------\n");
    // std::stringstream xxx;
    // xxx << root2;
    // // Not work..................
    // // YAML::Node str_to_node = trycle::LexicalCast<std::string, YAML::Node>()(xxx.str());
    // // std::cout << "ss......size=" << xxx.size() << "\n";
    // // const std::vector<YAML::Node> str_to_nodes = trycle::LexicalCast<std::string, std::vector<YAML::Node>>()(nodes_to_str);
    // // LOG_FMT_DEBUG(GET_ROOT_LOGGER, "str_to_nodes size = %d", (int)str_to_nodes.size());

    printf("----------------------\n");

    return 0;
}