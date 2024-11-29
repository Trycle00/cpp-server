#include "address.h"
#include "endianx.h"
#include "log.h"

#include <sstream>

namespace trycle
{

static auto g_logger = GET_LOGGER("system");

template <typename T>
static T CreateMask(int32_t bits)
{
    return ((1 << sizeof(T) * 8 - bits)) - 1;
}

/**
 * ============================================================================
 * Address 类的实现
 * ============================================================================
 */

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

Ipv4Address::Ipv4Address()
{
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin_family = AF_INET;
}

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
