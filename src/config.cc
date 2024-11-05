#include "config.h"
#include <iostream>

namespace trycle
{

// Config::ConfigVarMap Config::m_datas;

// std::ostream& operator<<(std::ostream& out, const ConfigVarBase& operand)
// {
//     out << operand.toString();
//     return out;
// }

std::ostream& operator<<(std::ostream& out, const ConfigVarBase& cvb)
{
    // out << cvb.getName() << ": " << cvb.toString();
    return out;
}

// static void loadFromYaml()
// {

//     // return nullptr;
// }

} // namespace trycle
