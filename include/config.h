#ifndef TRY_CONFIG_H
#define TRY_CONFIG_H

#include <algorithm>
#include <boost/lexical_cast.hpp>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <vector>
#include <yaml-cpp/yaml.h>

#include "log.h"

namespace trycle
{

class ConfigVarBase
{

public:
    typedef std::shared_ptr<ConfigVarBase> ptr;

    ConfigVarBase(const std::string& name,
                  const std::string& var_description = "")
        : m_name(name),
          m_var_description(var_description)
    {
    }

    virtual std::string toString() const            = 0;
    virtual bool fromString(const std::string& str) = 0;

    std::string get_var_name() const
    {
        return m_name;
    }

protected:
    std::string m_name;
    std::string m_var_description;
};

/**
 * @brief YAML格式字符串到其他类型的转换仿函数
 * boost::lexical_cast的包装
 * 因为boost::lexical_cast是使用std::stringstream实现的类型转换，
 * 所以仅支持实现了ostream::operator<<与isstream::operator>>的类型，
 * 可以说默认情况下仅支持std::string与各类Number类型的双向转换。
 * 需要转换自定义的类型，可以选择实现对应类型的流操作，或者将模板类进行偏特化。 *
 */
template <typename Source, typename Target>
class LexicalCast
{

public:
    Target operator()(const Source& source)
    {
        return boost::lexical_cast<Target>(source);
    }
};

template <typename T>
class LexicalCast<std::string, std::vector<T>>
{
public:
    std::vector<T> operator()(const std::string& str)
    {
        YAML::Node node = YAML::Load(str);
        typename std::vector<T> vec;
        if (node.IsSequence())
        {
            std::stringstream ss;
            for (const auto& item : node)
            {
                ss.str(""); // reset
                ss << item;
                // std::cout << "@@@@@@@@@@@...vector....item=" << item << std::endl;
                vec.push_back(LexicalCast<std::string, T>()(ss.str()));
            }
        }
        return vec;
    }
};

template <typename T>
class LexicalCast<std::vector<T>, std::string>
{
public:
    std::string operator()(const std::vector<T> vec)
    {
        YAML::Node node;
        for (const auto& n : vec)
        {
            node.push_back(LexicalCast<T, std::string>()(n));
        }

        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

template <typename T>
class LexicalCast<std::string, std::list<T>>
{
public:
    std::list<T> operator()(const std::string& str)
    {
        YAML::Node node = YAML::Load(str);
        typename std::list<T> vec;
        if (node.IsSequence())
        {
            std::stringstream ss;
            for (const auto& item : node)
            {
                ss.str(""); // reset
                ss << item;
                vec.push_back(LexicalCast<std::string, T>()(ss.str()));
            }
        }
        return vec;
    }
};

template <typename T>
class LexicalCast<std::list<T>, std::string>
{
public:
    std::string operator()(const std::list<T> vec)
    {
        YAML::Node node;
        for (const auto& n : vec)
        {
            node.push_back(LexicalCast<T, std::string>()(n));
        }

        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

template <typename T>
class LexicalCast<std::set<T>, std::string>
{
public:
    std::string operator()(const std::set<T>& list)
    {
        YAML::Node node;
        for (const auto& i : list)
        {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }

        std::stringstream ss;
        ss << node;

        return ss.str();
    }
};

template <typename T>
class LexicalCast<std::string, std::set<T>>
{
public:
    std::set<T> operator()(const std::string& str)
    {
        std::set<T> list;
        YAML::Node node = YAML::Load(str);
        std::stringstream ss;
        for (const auto& n : node)
        {
            ss.str("");
            ss << n;
            list.insert(LexicalCast<std::string, T>()(ss.str()));
        }

        return list;
    }
};

template <typename T>
class LexicalCast<std::map<std::string, T>, std::string>
{
public:
    std::string operator()(const std::map<std::string, T>& map)
    {
        YAML::Node node;
        for (const auto& it : map)
        {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(it.second)));
        }

        std::stringstream ss;
        ss << node;

        return ss.str();
    }
};

template <typename T>
class LexicalCast<std::string, std::map<std::string, T>>
{
public:
    std::map<std::string, T> operator()(const std::string& str)
    {
        std::map<std::string, T> map;

        YAML::Node node = YAML::Load(str);
        std::stringstream ss;
        for (const auto& n : node)
        {
            ss.str("");
            ss << n.second;
            std::string resstr = ss.str();
            // std::cout << "@@@@@@@@@@@...map..key=" << n.first << std::endl;
            // std::cout << "@@@@@@@@@@@...map..resstr=" << resstr << std::endl;
            map.insert(std::make_pair(n.first.as<std::string>(), LexicalCast<std::string, T>()(resstr)));
        }

        return map;
    }
};

template <>
class LexicalCast<std::string, LogAppenderConfig>
{

public:
    LogAppenderConfig operator()(const std::string& str)
    {
        YAML::Node node = YAML::Load(str);
        LogAppenderConfig config;

        config.type = node["type"].as<int>();
        if (node["level"])
        {
            config.level = static_cast<LogLevel::Level>(node["level"].as<int>());
        }
        if (node["file"])
        {
            config.file = node["file"].as<std::string>();
        }

        return config;
    }
};
template <>
class LexicalCast<LogAppenderConfig, std::string>
{
public:
    std::string operator()(const LogAppenderConfig& config)
    {
        YAML::Node node;

        node["type"]  = config.type;
        node["level"] = (int)config.level;
        node["file"]  = config.file;

        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

template <>
class LexicalCast<std::string, LogConfig>
{
public:
    LogConfig operator()(const std::string& str)
    {
        YAML::Node node = YAML::Load(str);
        LogConfig log_config;
        log_config.name      = node["name"].as<std::string>();
        log_config.formatter = node["formatter"].as<std::string>();
        log_config.level     = static_cast<LogLevel::Level>(node["level"].as<int>());
        auto appender        = node["appender"];
        if (appender)
        {
            std::stringstream ss;
            ss << appender;
            log_config.appenders = LexicalCast<std::string, std::set<LogAppenderConfig>>()(ss.str());
        }
        return log_config;
    }
};

template <>
class LexicalCast<LogConfig, std::string>
{
public:
    std::string operator()(const LogConfig& config)
    {
        YAML::Node node;

        std::stringstream ss;

        node["name"]  = config.name;
        node["level"] = 1; // TODO config.level;
        node["name"]  = config.formatter;

        // if (config.appenders)
        // {
        // for (const auto& item : config.appenders)
        // {
        //     ss.str("");
        //     ss << item;
        //     node["apppender"] = ss.str();
        // }
        // }
        node["appender"] = YAML::Load(LexicalCast<std::set<LogAppenderConfig>, std::string>()(config.appenders));

        ss.str("");
        ss << node;
        return ss.str();
    }
};

template <class T,
          class ToStringFN   = LexicalCast<T, std::string>,
          class FromStringFN = LexicalCast<std::string, T>>
class ConfigVar : public ConfigVarBase
{
    // friend std::ostream& operator<< (std::ostream&out, const ConfigVar<T> operand);
public:
    typedef std::shared_ptr<ConfigVar<T>> ptr;
    typedef std::function<void(const T& old_val, const T& new_val)> on_change_cb;

    ConfigVar(const std::string& name, const T& var_val, const std::string& description = "")
        : ConfigVarBase(name, description),
          m_val(var_val)
    {
    }

    // template <typename M>
    std::string toString() const override
    {
        try
        {
            // std::string cast_val = boost::lexical_cast<std::string>(m_val);
            // return m_name + "=" + cast_val;
            // return cast_val;
            // return m_name + "=" + boost::lexical_cast<std::string>(m_val);
            // return boost::lexical_cast<std::string>(m_val);
            return ToStringFN()(getVal());
        }
        catch (std::exception& e)
        {
            printf("ConfigVar::toString Exception | %s | convert %s to string.\n",
                   e.what(), typeid(m_val).name());
            throw std::bad_cast();
        }
        return "<error>";
    }

    // template <typename M>
    bool fromString(const std::string& str) override
    {
        try
        {
            // m_val = boost::lexical_cast<T>(str);
            setVal(FromStringFN()(str));
            return true;
        }
        catch (const std::exception& e)
        {
            printf("ConfigVar::fromString exception | %s | convert string to %s | %s.\n",
                   e.what(), typeid(m_val).name(), str.c_str());
            throw std::bad_cast();
        }
        return false;
    }

    void add_listener(const int key, const on_change_cb cb)
    {
        // m_cbs.insert(std::make_pair(key, cb));
        m_cbs[key] = cb;
    }

    on_change_cb get_listener(const int key)
    {
        auto it = m_cbs.find(key);
        return it == m_cbs.end() ? nullptr : it.second;
    }

    T getVal() const
    {
        return m_val;
    }

    void setVal(const T& val)
    {
        if (m_val == val)
        {
            return;
        }
        for (const auto& pair : m_cbs)
        {
            pair.second(m_val, val);
        }
        m_val = val;
    }

private:
    T m_val;
    std::map<int, on_change_cb> m_cbs;
};

/**
 * 配置思想：约定优于配置，即，优先使用代码所传默认参数；加载配置时，只加载有默认参数的配置
 */
class Config
{
public:
    typedef std::shared_ptr<Config> ptr;
    typedef std::map<std::string, ConfigVarBase::ptr> ConfigVarMap;

    static ConfigVarBase::ptr lookUp(const std::string& var_name)
    {
        ConfigVarMap& var_map = getDatas();
        auto itor             = var_map.find(var_name);
        if (itor != var_map.end())
        {
            // LOG_FMT_INFO(GET_ROOT_LOGGER, "Found config val %s", var_name.c_str());
            return itor->second;
        }
        return nullptr;
    }

    template <class T>
    static typename ConfigVar<T>::ptr lookUp(const std::string& var_name)
    {
        auto val = lookUp(var_name);
        if (!val)
        {
            return nullptr;
        }

        auto ptr = std::dynamic_pointer_cast<ConfigVar<T>>(val);
        if (!ptr)
        {
            printf("Config::lookUp() exception | Can not cast value to T of template\n");
            throw std::bad_cast();
        }
        return ptr;
    }

    template <class T>
    static typename ConfigVar<T>::ptr lookUp(const std::string& var_name, const T& default_val, const std::string& description = "")
    {
        // typename ConfigVar<T>::ptr val = lookUp(var_name);
        auto val = lookUp<T>(var_name);
        if (val)
        {
            return val;
        }

        if (var_name.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789._") != std::string::npos)
        {
            printf("Var name must start with letter, num,  _ or .\n");
            throw std::bad_cast();
        }

        auto new_val = std::make_shared<ConfigVar<T>>(var_name, default_val, description);
        // getDatas().insert({var_name, new_val});
        getDatas()[var_name] = new_val;

        return new_val;
    }

    static void loadFromYAML(const YAML::Node& root)
    {
        std::vector<std::pair<std::string, YAML::Node>> node_list;
        traversalNode(root, "", node_list);
        for (const auto& pair : node_list)
        {
            std::string key = pair.first;
            if (key.empty())
            {
                continue;
            }

            std::transform(key.begin(), key.end(), key.begin(), ::tolower);
            auto var = lookUp(key);
            if (var)
            {

                std::stringstream ss;
                ss << pair.second;
                std::string strRes = ss.str();
                var->fromString(strRes);
            }
        }
    }

private:
    static void traversalNode(const YAML::Node& node, const std::string& name, std::vector<std::pair<std::string, YAML::Node>>& output)
    {
        auto output_itor = std::find_if(output.begin(), output.end(), [&name](const std::pair<std::string, YAML::Node>& val)
                                        { return val.first == name; });
        if (output_itor != output.end())
        {
            output_itor->second = node;
        }
        else
        {
            output.push_back(std::make_pair(name, node));
        }

        if (node.IsSequence())
        {
            for (size_t i{}; i < node.size(); i++)
            {
                traversalNode(node[i], name + "." + std::to_string(i), output);
            }
        }
        if (node.IsMap())
        {
            for (auto itor = node.begin(); itor != node.end(); itor++)
            {
                traversalNode(itor->second, name.empty() ? itor->first.Scalar() : name + "." + itor->first.Scalar(), output);
            }
        }
    }

    static ConfigVarMap& getDatas()
    {
        static ConfigVarMap m_datas;
        return m_datas;
    }
};

std::ostream& operator<<(std::ostream& out, const ConfigVarBase& operand);

} // namespace trycle

#endif // TRY_CONFIG_H