// #include <stdint.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <string>
// #include <typeinfo>
// #include <yaml-cpp/yaml.h>
// #include <sstream>

// #include "config.h"
// #include "log.h"
// #include "util.h"

// void print_yaml(YAML::Node node, int level)
// {
//     if (node.IsScalar())
//     {
//         LOG_FMT_DEBUG(GET_ROOT_LOGGER, "%s--%d : %s", std::string(level * 2, ' ').c_str(), level, node.Scalar().c_str());
//     }
//     else if (node.IsNull())
//     {
//         // LOG_FMT_DEBUG(GET_ROOT_LOGGER, "%s--%d--%s: ", std::string(level * 2, ' ').c_str(), level, static_cast<std::string>(node[i].Type()).c_str());
//     }
//     else if (node.IsSequence())
//     {
//         LOG_FMT_DEBUG(GET_ROOT_LOGGER, "%s--%d--%d", std::string(level * 2, ' ').c_str(), level, node.Type());
//         for (size_t i{}; i < node.size(); i++)
//         {
//             auto n = node[i];
//             if (n.IsMap() || n.IsSequence())
//             {
//                 print_yaml(n, level + 1);
//                 continue;
//             }
//             LOG_FMT_DEBUG(GET_ROOT_LOGGER, "%s--%d: %s", std::string(level * 2, ' ').c_str(), level, n.as<std::string>().c_str());
//         }
//     }
//     else if (node.IsMap())
//     {
//         for (auto itor = node.begin(); itor != node.end(); itor++)
//         {
//             LOG_FMT_DEBUG(GET_ROOT_LOGGER, "%s--%d--%d", std::string(level * 2, ' ').c_str(), level, node.Type());
//             if (itor->second.IsMap() || itor->second.IsSequence())
//             {
//                 LOG_FMT_DEBUG(GET_ROOT_LOGGER, "%s--%d--%s:", std::string(level * 2, ' ').c_str(), level, itor->first.as<std::string>().c_str());
//                 print_yaml(itor->second, level + 1);
//                 continue;
//             }
//             LOG_FMT_DEBUG(GET_ROOT_LOGGER, "%s--%d--%s: %s", std::string(level * 2, ' ').c_str(), level, itor->first.as<std::string>().c_str(), itor->second.as<std::string>().c_str());
//         }
//     }
// }


// trycle::ConfigVar<int>::ptr int_val = trycle::Config::lookUp("test.int_val", 1);
// auto int_vec = trycle::Config::lookUp("test.int_vec", std::vector<int>{1,2,3}, "");



// int main(int argc, char** argv)
// {
//     printf("1======================\n");
//     printf("TEST_defaultLogger:\n");
//     // // LogFormatter log_formatter("%p %s %t asdf %% %l %f");
//     // auto logger = std::make_shared<Logger>("root", LogLevel::INFO, std::make_shared<LogFormatter>("%d [%p] %f:%l [%t-%F] - %m%n"));

//     // logger->addAppender(std::make_shared<StdoutAppender>());
//     // logger->addAppender(std::make_shared<FileAppender>("FileAppender"));

//     // logger.info(std::make_shared<LogEvent>(__FILE__, __LINE__,  1, 2, time(0), "Hello..world"));

//     printf("2----------------------\n");
//     printf("TEST_macroLogger:\n");

//     auto logger = std::make_shared<trycle::Logger>("root", trycle::LogLevel::DEBUG, std::make_shared<trycle::LogFormatter>("%d [%p] %f:%l [%t-%F] - %m%n"));

//     logger->addAppender(std::make_shared<trycle::StdoutAppender>());
//     logger->addAppender(std::make_shared<trycle::FileAppender>("FileAppender"));
//     auto e = MAKE_LOG_EVENT(trycle::LogLevel::Level::DEBUG, "Hello 222222222222");
//     // logger->log( MAKE_LOG_EVENT(trycle::LogLevel::Level::DEBUG, "Hello 222222222222") );

//     LOG_LEVEL(logger, trycle::LogLevel::Level::DEBUG, "Hello 222222222222");
//     LOG_DEBUG(logger, "Hello 3333333333333");
//     LOG_INFO(logger, "Hello 44444444444");
//     LOG_WARN(logger, "Hello 555555555555");
//     LOG_ERROR(logger, "Hello 6666666666");
//     LOG_FATAL(logger, "Hello 777777777777");
//     LOG_FATAL(logger, "Hello 777777777777");
//     LOG_FATAL(logger, "Hello threadId=" + std::to_string(trycle::GetThreadId()));

//     printf("----------------------\n");
//     printf("TEST_LOG_FMT_LOGGER:\n");
//     LOG_FMT_ERROR(GET_ROOT_LOGGER, "ConfigVar::toString Exception | %s | convert %s to string.",
//                   "a1111", "b22222");

//     printf("----------------------\n");
//     printf("TEST_loggerManager:\n");

//     // trycle::LoggerManager->
//     auto test = trycle::LoggerManager::GetSingleton()->getLogger("test");
//     // trycle::LoggerManager->
//     LOG_DEBUG(test, "trycle::LoggerManager-> Hello 11111111111");

//     printf("----------------------\n");
//     printf("TEST_Config:\n");

//     trycle::ConfigVar<int>::ptr var1 = trycle::Config::lookUp("test.k.var1", (int)111, "var1 is 111");
//     LOG_FMT_DEBUG(GET_ROOT_LOGGER, "%s=%s", var1->get_var_name().c_str(), var1->toString().c_str());

//     trycle::ConfigVar<std::string>::ptr var2 = trycle::Config::lookUp("test.k.var2", std::string("aaa"), "var2 is aaa");
//     LOG_FMT_DEBUG(GET_ROOT_LOGGER, "%s=%s", var2->get_var_name().c_str(), var2->toString().c_str());

//     printf("----------------------\n");
//     printf("TEST_YAML:\n");

//     YAML::Node root = YAML::LoadFile("./conf/config.yaml");
//     print_yaml(root, 0);

//     LOG_FMT_DEBUG(GET_ROOT_LOGGER, "node.size(): %d", (int)root.size());

//     printf("----------------------\n");

//     printf("TEST_config:\n");

//     // trycle::Config::lookUp<std::string>("system.port", "11", "");
//     LOG_FMT_DEBUG(GET_ROOT_LOGGER, "Before: %s", trycle::Config::lookUp<std::string>("system.port", "11")->toString().c_str());
//     LOG_FMT_DEBUG(GET_ROOT_LOGGER, "Before: %s", trycle::Config::lookUp<std::string>("system.name", "22")->toString().c_str());
//     YAML::Node root2 = YAML::LoadFile("conf/config.yaml");
//     trycle::Config::loadFromYAML(root2);

//     LOG_FMT_DEBUG(GET_ROOT_LOGGER, "After: %s", trycle::Config::lookUp("system.port")->toString().c_str());
//     LOG_FMT_DEBUG(GET_ROOT_LOGGER, "After: %s", trycle::Config::lookUp("system.name")->toString().c_str());

//     printf("----------------------\n");

//     printf("TEST_LexicalCast:\n");

//     std::string ss = trycle::LexicalCast<int, std::string>()(1);
//     LOG_FMT_DEBUG(GET_ROOT_LOGGER, "trycle::LexicalCast<int, std::string>()(1) = %s", ss.c_str());
//     std::vector<YAML::Node> nodes{root2};
//     const std::string nodes_to_str = trycle::LexicalCast<std::vector<YAML::Node>, std::string>()(nodes);
//     LOG_FMT_DEBUG(GET_ROOT_LOGGER, "nodes_to_str = %s", nodes_to_str.c_str());

//     printf("------------\n");
//     std::cout << std::endl;
//     std::cout << root2 << "\n";

//     printf("------------\n");
//     std::stringstream xxx;
//     xxx << root2;
//     // Not work..................
//     // YAML::Node str_to_node = trycle::LexicalCast<std::string, YAML::Node>()(xxx.str());
//     // std::cout << "ss......size=" << xxx.size() << "\n";
//     // const std::vector<YAML::Node> str_to_nodes = trycle::LexicalCast<std::string, std::vector<YAML::Node>>()(nodes_to_str);
//     // LOG_FMT_DEBUG(GET_ROOT_LOGGER, "str_to_nodes size = %d", (int)str_to_nodes.size());

//     printf("----------------------\n");

//     return 0;
// }