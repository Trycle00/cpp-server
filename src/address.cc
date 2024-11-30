#include "address.h"
#include "endianx.h"
#include "log.h"

#include <ifaddrs.h>
#include <netdb.h>
#include <sstream>

namespace trycle
{

static auto g_logger = GET_LOGGER("system");

template <typename T>
static T CreateMask(int32_t bits)
{
    return ((1 << sizeof(T) * 8 - bits)) - 1;
}

template <typename T>
static uint32_t CountBytes(T val)
{
    uint32_t result = 0;
    for (; val; ++result)
    {
        val &= val - 1;
    }
    return result;
}

/**
 * ============================================================================
 * Address 类的实现
 * ============================================================================
 */

Address::ptr Address::Create(const sockaddr* addr, socklen_t sock_len)
{
    if (addr == nullptr)
    {
        return nullptr;
    }
    Address::ptr result;
    switch (addr->sa_family)
    {
        case AF_INET:
            result.reset(new Ipv4Address(*(sockaddr_in*)addr));
            break;
        case AF_INET6:
            result.reset(new Ipv6Address(*(sockaddr_in6*)addr));
            break;

        default:
            result.reset(new UnknownAddress(*addr));
            break;
    }
    return result;
}

bool Address::Lookup(std::vector<Address::ptr>& result, const std::string& host, int family, int type, int protoccol)
{
    addrinfo hints, *results, *next;
    memset(&hints, 0, sizeof(hints));
    hints.ai_flags     = 0;
    hints.ai_family    = family;
    hints.ai_socktype  = type,
    hints.ai_protocol  = protoccol;
    hints.ai_addrlen   = 0;
    hints.ai_canonname = NULL;
    hints.ai_addr      = NULL;
    hints.ai_next      = NULL;

    std::string node;
    const char* service = NULL;

    // 检查是否为 ipv6address service
    if (!host.empty() && host[0] == '[')
    {
        const char* endipv6 = (const char*)memchr(host.c_str() + 1, ']', host.size() - 1);
        if (endipv6)
        {
            if (*(endipv6 + 1) == ':')
            {
                service = endipv6 + 2;
            }
            node = host.substr(1, endipv6 - host.c_str() - 1);
        }
    }

    // 检查 node service
    if (node.empty())
    {
        service = (const char*)memchr(host.c_str(), ':', host.size());
        if (service)
        {
            if (!memchr(service + 1, ':', host.c_str() + host.size() - service - 1))
            {
                node = host.substr(0, service - host.c_str());
                ++service;
            }
        }
    }

    if (node.empty())
    {
        node = host;
    }

    int error = getaddrinfo(node.c_str(), service, &hints, &results);
    if (error)
    {
        LOG_FMT_ERROR(g_logger, "Address::Lookup error | host=%s, family=%d, type=%d | error=%d, errstr=%s",
                      host.c_str(), family, type, error, gai_strerror(error));
        return false;
    }

    next = results;
    while (next)
    {
        result.push_back(Create(next->ai_addr, next->ai_addrlen));
        next = next->ai_next;
    }

    freeaddrinfo(results);

    return !result.empty();
}

Address::ptr Address::LookupAny(const std::string& host, int family, int type, int protocol)
{
    std::vector<Address::ptr> result;
    bool rt = Lookup(result, host, family, type, protocol);
    if (rt)
    {
        return result[0];
    }
    return nullptr;
}

std::shared_ptr<IpAddress> Address::LookupAnyIpAddress(const std::string& host, int family, int type, int protocol)
{
    std::vector<Address::ptr> result;
    if (Lookup(result, host, family, type, protocol))
    {
        for (auto& item : result)
        {
            auto rt = std::dynamic_pointer_cast<IpAddress>(item);
            if (rt)
            {
                return rt;
            }
        }
    }
    return nullptr;
}

bool Address::GetInterfaceAddress(std::multimap<std::string, std::pair<Address::ptr, uint32_t>>& result, int family)
{
    struct ifaddrs hints, *results, *next;
    if (getifaddrs(&results) != 0)
    {
        LOG_FMT_ERROR(g_logger, "Address::GetInterfaceAddress getifaddr error | errno=%d, errstr=%s", errno, strerror(errno));
        return false;
    }

    try
    {

        for (next = results; next = next->ifa_next;)
        {
            Address::ptr addr;
            uint32_t prefix_len = ~0u;
            if (family != AF_UNSPEC && family != next->ifa_addr->sa_family)
            {
                continue;
            }

            switch (next->ifa_addr->sa_family)
            {
                case AF_INET:
                {
                    addr             = Create(next->ifa_addr, sizeof(sockaddr_in));
                    uint32_t netmask = ((sockaddr_in*)next->ifa_netmask)->sin_addr.s_addr;
                    prefix_len       = CountBytes(netmask);
                }
                break;
                case AF_INET6:
                {
                    addr              = Create(next->ifa_addr, sizeof(sockaddr_in6));
                    in6_addr& netmask = ((sockaddr_in6*)next->ifa_netmask)->sin6_addr;
                    prefix_len        = 0;
                    for (int i = 0; i < 16; i++)
                    {
                        prefix_len += CountBytes(netmask.s6_addr[i]);
                    }
                }
                break;

                default:
                    break;
            }

            if (addr)
            {
                result.insert(std::make_pair(next->ifa_name, std::make_pair(addr, prefix_len)));
            }
        }
    }
    catch (...)
    {
        LOG_ERROR(g_logger, "Address::GetInterfaceAddress error");
        freeifaddrs(results);
        return false;
    }

    freeifaddrs(results);
    return !result.empty();
}

bool Address::GetInterfaceAddresses(std::vector<std::pair<Address::ptr, uint32_t>>& result, const std::string& iface, int family)
{
    if (iface.empty() || iface == "*")
    {
        if (family == AF_INET || family == AF_UNSPEC)
        {
            result.push_back(std::make_pair(Address::ptr(new Ipv4Address()), 0u));
        }
        if (family == AF_INET6 || family == AF_UNSPEC)
        {
            result.push_back(std::make_pair(Address::ptr(new Ipv6Address()), 0u));
        }
        return true;
    }

    std::multimap<std::string, std::pair<Address::ptr, uint32_t>> results;
    if (!GetInterfaceAddress(results, family))
    {
        return false;
    }

    auto its = results.equal_range(iface);
    for (; its.first != its.second; ++its.first)
    {
        result.push_back(its.first->second);
    }
    return !result.empty();
}

int Address::getFamily() const
{
    return getAddr()->sa_family;
}

std::string Address::toString() const
{
    std::stringstream ss;
    insert(ss);
    return ss.str();
}

bool Address::operator<(const Address& rhs) const
{
    socklen_t cmp_len = std::min(getAddrLen(), rhs.getAddrLen());
    int rt            = memcmp(getAddr(), rhs.getAddr(), cmp_len);
    if (rt > 0)
    {
        return false;
    }
    else if (rt < 0)
    {
        return true;
    }
    return getAddrLen() < rhs.getAddrLen();
}

bool Address::operator==(const Address& rhs) const
{
    return getAddrLen() == rhs.getAddrLen() &&
           memcmp(getAddr(), rhs.getAddr(), getAddrLen());
}

bool Address::operator!=(const Address& rhs) const
{
    return !(*this == rhs);
}

/**
 * ============================================================================
 * IpAddress 类的实现
 * ============================================================================
 */

IpAddress::ptr IpAddress::Create(const char* addr, uint16_t port)
{
    addrinfo hints, *results;
    memset(&hints, 0, sizeof(addrinfo));

    hints.ai_flags  = AI_NUMERICHOST;
    hints.ai_family = AF_UNSPEC;

    int error       = getaddrinfo(addr, NULL, &hints, &results);
    if (error)
    {
        LOG_FMT_ERROR(g_logger, "Ipv4Address::Create error | addr=%s, port%d | error=%d, errstr=%s",
                      addr, port, errno, strerror(errno));
        return nullptr;
    }

    try
    {
        IpAddress::ptr result = std::dynamic_pointer_cast<IpAddress>(Address::Create(results->ai_addr, results->ai_addrlen));
        if (result)
        {
            result->setPort(port);
        }
        freeaddrinfo(results);
        return result;
    }
    catch (...)
    {
        LOG_ERROR(g_logger, "IpAddress::Create error.");
        freeaddrinfo(results);
        return nullptr;
    }
}

// IpAddress::ptr IpAddress::broadcastAddress(uint32_t prefix_len)
// {
// }

// IpAddress::ptr IpAddress::networkAddress(uint32_t refix_len)
// {
// }

// IpAddress::ptr IpAddress::subnetMask(uint32_t prefix_len)
// {
// }

// uint16_t IpAddress::getPort() const
// {
//     return m_port;
// }

// void IpAddress::setPort(uint16_t port)
// {
//     m_port = port;
// }

/**
 * ============================================================================
 * Ipv4Address 类的实现
 * ============================================================================
 */

Ipv4Address::ptr Ipv4Address::Create(const char* addr, uint16_t port)
{
    Ipv4Address::ptr rt(new Ipv4Address);
    rt->m_addr.sin_port = byteswapOnLittleEndian(port);
    // 将点分十进制地址转换成 uint32_t 值
    int pton_result = inet_pton(AF_INET, addr, &rt->m_addr.sin_addr);
    if (pton_result <= 0)
    {
        LOG_FMT_ERROR(g_logger, "Ipv4Address::Create error | addr=%s, port=%d, | errno=%d, errstr=%s",
                      addr, port, errno, strerror(errno));
        return nullptr;
    }

    return rt;
}

// Ipv4Address::Ipv4Address()
// {
//     memset(&m_addr, 0, sizeof(m_addr));
//     m_addr.sin_family = AF_INET;
// }

Ipv4Address::Ipv4Address(const sockaddr_in& address)
{
    m_addr = address;
}

Ipv4Address::Ipv4Address(uint32_t address, uint16_t port)
{
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin_family      = AF_INET;
    m_addr.sin_addr.s_addr = byteswapOnLittleEndian(address);
    m_addr.sin_port        = byteswapOnLittleEndian(port);
}

sockaddr* Ipv4Address::getAddr() const
{
    return (sockaddr*)&m_addr;
}

socklen_t Ipv4Address::getAddrLen() const
{
    return sizeof(m_addr);
}

std::ostream& Ipv4Address::insert(std::ostream& out) const
{
    uint32_t addr = byteswapOnLittleEndian(m_addr.sin_addr.s_addr);
    out << "["
        << (addr >> 24 & 0xff) << '.'
        << (addr >> 16 & 0xff) << '.'
        << (addr >> 8 & 0xff) << '.'
        << (addr & 0xff)
        << "]:" << byteswapOnLittleEndian(m_addr.sin_port);
    return out;
}

IpAddress::ptr Ipv4Address::broadcastAddress(uint32_t prefix_len)
{
    if (prefix_len > 32)
    {
        return nullptr;
    }

    sockaddr_in baddr(m_addr);
    baddr.sin_addr.s_addr |= byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len));

    return Ipv4Address::ptr(new Ipv4Address(baddr));
}

IpAddress::ptr Ipv4Address::networkAddress(uint32_t prefix_len)
{
    if (prefix_len > 32)
    {
        return nullptr;
    }

    sockaddr_in baddr(m_addr);
    baddr.sin_addr.s_addr &= byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len));
    return Ipv4Address::ptr(new Ipv4Address(baddr));
}

IpAddress::ptr Ipv4Address::subnetMask(uint32_t prefix_len)
{
    sockaddr_in subnet;
    memset(&subnet, 0, sizeof(subnet));
    subnet.sin_family      = AF_INET;
    subnet.sin_addr.s_addr = ~byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len));

    return Ipv4Address::ptr(new Ipv4Address(subnet));
}

uint16_t Ipv4Address::getPort() const
{
    return byteswapOnLittleEndian(m_addr.sin_port);
}

void Ipv4Address::setPort(uint16_t port)
{
    m_addr.sin_port = byteswapOnLittleEndian(port);
}

/**
 * ============================================================================
 * Ipv6Address 类的实现
 * ============================================================================
 */

Ipv6Address::ptr Ipv6Address::Create(const char* addr, uint16_t port)
{
    Ipv6Address::ptr rt(new Ipv6Address);

    rt->m_addr.sin6_port = byteswapOnLittleEndian(port);

    int pton_result      = inet_pton(AF_INET6, addr, &rt->m_addr.sin6_addr);
    if (pton_result <= 0)
    {
        LOG_FMT_ERROR(g_logger, "Ipv6Address::create error | addr=%s, port=%d | errno=%d, errstr=%s",
                      addr, port, errno, strerror(errno));
        return nullptr;
    }

    return rt;
}

Ipv6Address::Ipv6Address()
{
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin6_family = AF_INET6;
}

Ipv6Address::Ipv6Address(const sockaddr_in6& address)
{
    m_addr = address;
}

Ipv6Address::Ipv6Address(const uint8_t address[16], uint16_t port)
{
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin6_family = AF_INET6;
    m_addr.sin6_port   = byteswapOnLittleEndian(port);
    memcpy(&m_addr.sin6_addr.s6_addr, address, 16);
}

sockaddr* Ipv6Address::getAddr() const
{
    return (sockaddr*)&m_addr;
}

socklen_t Ipv6Address::getAddrLen() const
{
    return sizeof(m_addr);
}

std::ostream& Ipv6Address::insert(std::ostream& out) const
{
    out << "[";
    uint16_t* addr  = (uint16_t*)m_addr.sin6_addr.s6_addr;
    bool used_zeros = false;
    for (int i = 0; i < 8; i++)
    {
        if (addr[i] == 0 && !used_zeros)
        {
            continue;
        }
        if (i && addr[i - 1] == 0 && !used_zeros)
        {
            out << ":";
            used_zeros = true;
            continue;
        }
        if (i)
        {
            out << ":";
        }
        out << std::hex << byteswapOnLittleEndian(addr[i]) << std::dec;
    }
    if (!used_zeros && addr[7] == 0)
    {
        out << "::";
    }
    out << "]:" << byteswapOnLittleEndian(m_addr.sin6_port);
    return out;
}

IpAddress::ptr Ipv6Address::broadcastAddress(uint32_t prefix_len)
{
    sockaddr_in6 baddr(m_addr);
    baddr.sin6_addr.s6_addr[prefix_len / 8] |= CreateMask<uint8_t>(prefix_len % 8);
    for (int i = prefix_len / 8 + 1; i < 16; i++)
    {
        baddr.sin6_addr.s6_addr[i] = 0xff;
    }
    return Ipv6Address::ptr(new Ipv6Address(baddr));
}

IpAddress::ptr Ipv6Address::networkAddress(uint32_t prefix_len)
{
    sockaddr_in6 baddr(m_addr);
    baddr.sin6_addr.s6_addr[prefix_len / 8] &= CreateMask<uint8_t>(prefix_len % 8);
    for (int i = prefix_len / 8 + 1; i < 16; i++)
    {
        baddr.sin6_addr.s6_addr[i] = 0x00;
    }

    return Ipv6Address::ptr(new Ipv6Address(baddr));
}

IpAddress::ptr Ipv6Address::subnetMask(uint32_t prefix_len)
{
    sockaddr_in6 subnet(m_addr);
    subnet.sin6_addr.s6_addr[prefix_len / 8] = ~CreateMask<uint8_t>(prefix_len % 8);
    for (int i = 0; i < prefix_len / 8; i++)
    {
        subnet.sin6_addr.s6_addr[i] = 0xff;
    }
    return Ipv6Address::ptr(new Ipv6Address(subnet));
}

uint16_t Ipv6Address::getPort() const
{
    return byteswapOnLittleEndian(m_addr.sin6_port);
}

void Ipv6Address::setPort(uint16_t port)
{
    m_addr.sin6_port = byteswapOnLittleEndian(port);
}

/**
 * ============================================================================
 * UnixAddress 类的实现
 * ============================================================================
 */
static const size_t MAX_PATH_LEN = sizeof(((sockaddr_un*)0)->sun_path) - 1;

UnixAddress::UnixAddress()
{
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sun_family = AF_UNIX;
    m_socklen         = offsetof(sockaddr_un, sun_path) + MAX_PATH_LEN;
}

UnixAddress::UnixAddress(const std::string& path)
{
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sun_family = AF_UNIX;
    m_socklen         = path.size() + 1;
    if (!path.empty() && path[0] == '\0')
    {
        --m_socklen;
    }
    if (m_socklen > sizeof(m_addr.sun_path))
    {
        throw std::logic_error("path too long : " + path);
    }
    memcpy(m_addr.sun_path, path.c_str(), m_socklen);
    m_socklen += offsetof(sockaddr_un, sun_path);
}

sockaddr* UnixAddress::getAddr() const
{
    return (sockaddr*)&m_addr;
}

socklen_t UnixAddress::getAddrLen() const
{
    return m_socklen;
}

void UnixAddress::setAddrLen(socklen_t length)
{
    m_socklen = length;
}

std::string UnixAddress::getPath() const
{
    std::stringstream ss;
    if (m_socklen > offsetof(sockaddr_un, sun_path) &&
        m_addr.sun_path[0] == '\0')
    {
        ss << "\\0" << std::string(m_addr.sun_path + 1, m_socklen - offsetof(sockaddr_un, sun_path) - 1);
    }
    else
    {
        ss << m_addr.sun_path;
    }
    return ss.str();
}

std::ostream& UnixAddress::insert(std::ostream& out) const
{
    if (m_socklen > offsetof(sockaddr_un, sun_path) &&
        m_addr.sun_path[0] == '\0')
    {
        out << std::string(m_addr.sun_path + 1, m_socklen - offsetof(sockaddr_un, sun_path) - 1);
    }
    else
    {
        out << m_addr.sun_path;
    }
    return out;
}

/**
 * ============================================================================
 * UnknownAddress 类的实现
 * ============================================================================
 */
UnknownAddress::UnknownAddress(int family)
{
    memset(&m_addr, 0, sizeof(m_addr));
}

UnknownAddress::UnknownAddress(const sockaddr& address)
{
    m_addr = address;
}

sockaddr* UnknownAddress::getAddr() const
{
    return (sockaddr*)&m_addr;
}

socklen_t UnknownAddress::getAddrLen() const
{
    return sizeof(m_addr);
}

std::ostream& UnknownAddress::insert(std::ostream& out) const
{
    return out << "UnknownAddress";
}

std::ostream& operator<<(std::ostream& out, const Address& operand)
{
    return operand.insert(out);
}

} // namespace trycle
