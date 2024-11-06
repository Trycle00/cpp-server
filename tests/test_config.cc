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

void TEST_LexicalCast()
{

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

#define PRINT_M(config_var, prefix)                                                                                                                               \
    {                                                                                                                                                             \
        for (auto& pair : config_var->getVal())                                                                                                                   \
        {                                                                                                                                                         \
            LOG_FMT_DEBUG(GET_ROOT_LOGGER, #prefix " | %s : %s=%s", config_var->get_var_name().c_str(), pair.first.c_str(), std::to_string(pair.second).c_str()); \
        }                                                                                                                                                         \
    }

    PRINT_C(int_vec, "Before");
    PRINT_C(int_list, "Before");
    PRINT_C(int_set, "Before");
    PRINT_M(int_map, "Before");

    printf("-----------\n");

    TEST_loadConfig("../conf/config.yaml");
    printf("-----------\n");

    PRINT_C(int_vec, "After");
    PRINT_C(int_list, "After");
    PRINT_C(int_set, "After");
    PRINT_M(int_map, "After");
}

class Goods
{
    friend std::ostream& operator<<(std::ostream& out, const Goods& operand);

public:
    Goods() = default;
    Goods(std::string name,
          int num,
          int price)
        : m_name(name),
          m_num(num),
          m_price(price)
    {
    }

    std::string toString() const
    {
        char* buf   = nullptr;
        int written = asprintf(&buf, "Goods(m_name=%s, m_num=%d, m_price=%d", m_name.c_str(), m_num, m_price);
        if (written != -1)
        {
            std::string ss(buf);
            free(buf);
            return ss;
        }
        return std::string();
    }

    std::string get_name() { return m_name; }
    int get_num() { return m_num; }
    int get_price() { return m_price; }

public:
    std::string m_name;
    int m_num;
    int m_price;
};

std::ostream& operator<<(std::ostream& out, const Goods& operand)
{
    out << operand.toString();
    return out;
}

namespace trycle
{
template <>
class LexicalCast<std::string, Goods>
{
public:
    Goods operator()(const std::string& str)
    {
        YAML::Node node = YAML::Load(str);
        Goods goods;
        if (node.IsMap())
        {
            goods.m_name  = node["name"].as<std::string>();
            goods.m_num   = node["num"].as<int>();
            goods.m_price = node["price"].as<int>();
        }
        return goods;
    }
};

template <>
class LexicalCast<Goods, std::string>
{
public:
    std::string operator()(const Goods& goods)
    {
        YAML::Node node;
        node["name"]  = goods.m_name;
        node["num"]   = goods.m_num;
        node["price"] = goods.m_price;
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};
} // namespace trycle



auto g_goods          = trycle ::Config::lookUp("test.class.goods", Goods());
auto g_goods_map      = trycle ::Config::lookUp("test.class.goods_map", std::map<std::string, Goods>());
auto g_goods_map_list = trycle ::Config::lookUp("test.class.goods_map_list", std::map<std::string, std::vector<Goods>>());

void TEST_LexicalCast_Class()
{

    LOG_FMT_DEBUG(GET_ROOT_LOGGER, "g_goods : %s = %s", g_goods->get_var_name().c_str(), g_goods->getVal().toString().c_str());
    for (const auto& pair : g_goods_map->getVal())
    {
        LOG_FMT_DEBUG(GET_ROOT_LOGGER, "g_goods_map : %s : %s = %s", g_goods_map->get_var_name().c_str(), pair.first.c_str(), pair.second.toString().c_str());
    }
    for (const auto& pair : g_goods_map_list->getVal())
    {
        for (const auto& item : pair.second)
        {
            LOG_FMT_DEBUG(GET_ROOT_LOGGER, "g_goods_map_list : %s : %s = %s", g_goods_map->get_var_name().c_str(), pair.first.c_str(), item.toString().c_str());
        }
    }

    printf("-----------\n");
    TEST_loadConfig("../conf/config.yaml");
    printf("-----------\n");

    LOG_FMT_DEBUG(GET_ROOT_LOGGER, "g_goods : %s = %s", g_goods->get_var_name().c_str(), g_goods->getVal().toString().c_str());
    for (const auto& pair : g_goods_map->getVal())
    {
        LOG_FMT_DEBUG(GET_ROOT_LOGGER, "g_goods_map : %s : %s = %s", g_goods_map->get_var_name().c_str(), pair.first.c_str(), pair.second.toString().c_str());
    }
    for (const auto& pair : g_goods_map_list->getVal())
    {
        for (const auto& item : pair.second)
        {
            LOG_FMT_DEBUG(GET_ROOT_LOGGER, "g_goods_map_list : %s : %s = %s", g_goods_map->get_var_name().c_str(), pair.first.c_str(), item.toString().c_str());
        }
    }
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

    // TEST_LexicalCast();

    printf("----------------------\n");

    TEST_LexicalCast_Class();

    printf("----------------------\n");

    return 0;
}