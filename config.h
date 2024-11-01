#ifndef TRY_CONFIG_H
#define TRY_CONFIG_H

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

    virtual std::string toString() const      = 0;
    virtual bool fromString(std::string& str) = 0;

    std::string get_var_name() const
    {
        return m_name;
    }

protected:
    std::string m_name;
    std::string m_var_description;
};

template <typename T>
class ConfigVar : public ConfigVarBase
{
public:
    typedef std::shared_ptr<ConfigVar<T>> ptr;

    ConfigVar(const std::string& name, T& var_val)
        : m_val(var_val),
          ConfigVarBase(name)
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
            // LOG_FMT_ERROR(GET_ROOT, "ConfigVar::toString Exception | %s | convert %s to string.\n",
            //               e.what(), typeid(m_val).name());
        }
        return "<error>";
    }

    // template <typename M>
    bool fromString(std::string& str) override
    {
        try
        {
            T val = boost::lexical_cast<T>(str);
            m_val = val;
            return true;
        }
        catch (const std::exception& e)
        {
            // LOG_FMT_ERROR(GET_ROOT_LOGGER, "ConfigVar::fromString exception | %s | convert string to %s.\n",
            //               e.what(), typeid(m_val).name());
        }
        return false;
    }

    T& get_val()
    {
        return m_val;
    }

private:
    T& m_val;
};

class Config
{
public:
    typedef std::map<std::string, ConfigVarBase::ptr> ConfigVarMap;

private:
    static ConfigVarMap m_datas;
};

} // namespace trycle

#endif // TRY_CONFIG_H