#ifndef TRY_CONFIG_H
#define TRY_CONFIG_H

#include <algorithm>
#include <boost/lexical_cast.hpp>
#include <iostream>
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

template <class T>
class ConfigVar : public ConfigVarBase
{
    // friend std::ostream& operator<< (std::ostream&out, const ConfigVar<T> operand);
public:
    typedef std::shared_ptr<ConfigVar<T>> ptr;

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
            return m_name + "=" + boost::lexical_cast<std::string>(m_val);
        }
        catch (std::exception& e)
        {
            LOG_FMT_ERROR(GET_ROOT_LOGGER, "ConfigVar::toString Exception | %s | convert %s to string.\n",
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
            m_val = boost::lexical_cast<T>(str);
            return true;
        }
        catch (const std::exception& e)
        {
            LOG_FMT_ERROR(GET_ROOT_LOGGER, "ConfigVar::fromString exception | %s | convert string to %s.\n",
                          e.what(), typeid(m_val).name());
            throw std::bad_cast();
        }
        return false;
    }

private:
    T m_val;
};

class Config
{
public:
    typedef std::shared_ptr<Config> ptr;
    typedef std::map<std::string, ConfigVarBase::ptr> ConfigVarMap;

    static ConfigVarBase::ptr lookUp(const std::string& var_name)
    {
        auto itor = getDatas().find(var_name);
        if (itor != getDatas().end())
        {
            //LOG_FMT_INFO(GET_ROOT_LOGGER, "Found config val %s", var_name.c_str());
            return itor->second;
        }
        return nullptr;
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
            LOG_ERROR(GET_ROOT_LOGGER, "Var name must start with letter, num,  _ or .\n");
            throw std::bad_cast();
        }

        auto new_val = std::make_shared<ConfigVar<T>>(var_name, default_val, description);
        // getDatas().insert({var_name, new_val});
        getDatas()[var_name] = new_val;

        return new_val;
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
            LOG_ERROR(GET_ROOT_LOGGER, "Config::lookUp() exception | Can not cast value to T of template\n");
            throw std::bad_cast();
        }
        return ptr;
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
                var->fromString(ss.str());
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
        return m_datas;
    }

    static ConfigVarMap m_datas;
};

} // namespace trycle

#endif // TRY_CONFIG_H