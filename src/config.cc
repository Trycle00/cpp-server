#include "config.h"
#include <iostream>

namespace trycle
{

// Config::ConfigVarMap Config::m_datas;

std::ostream& operator<<(std::ostream& out, ConfigVarBase& operand)
{
    out << operand.get_var_name() << ": " << operand.toString();
    return out;
}

// std::ostream& operator<<(std::ostream& out, const ConfigVarBase& cvb)
// {
//     // out << cvb.getName() << ": " << cvb.toString();
//     return out;
// }

// static void loadFromYaml()
// {

//     // return nullptr;
// }

} // namespace trycle
