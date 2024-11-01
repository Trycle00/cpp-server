#ifndef TRY_CONFIG_H
#define TRY_CONFIG_H

#include <iostream>
#include <boost/lexical_cast.hpp>

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
            return boost::lexical_cast<std::string>(m_val);
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
    T m_val{};
};

class Config
{
public:
    typedef std::map<std::string, ConfigVarBase::ptr> ConfigVarMap;

    static ConfigVarBase::ptr lookUp(const std::string& var_name)
    {
        auto itor = getDatas().find(var_name);
        if (itor != getDatas().end())
        {
            LOG_FMT_INFO(GET_ROOT_LOGGER, "Found config val %s\n", var_name.c_str());
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

private:
    static ConfigVarMap& getDatas()
    {
        return m_datas;
    }

    static ConfigVarMap m_datas;
};

} // namespace trycle

#endif // TRY_CONFIG_H