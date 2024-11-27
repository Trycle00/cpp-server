#include "address.h"

#include <sstream>

namespace trycle
{

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

IpAddress::ptr IpAddress::broadcastAddress(uint32_t prefix_len)
{
}

IpAddress::ptr IpAddress::networkAddress(uint32_t refix_len)
{
}

IpAddress::ptr IpAddress::subnetMask(uint32_t prefix_len)
{
}

uint16_t IpAddress::getPort() const
{
}

void IpAddress::setPort(uint16_t port)
{
}

/**
 * ============================================================================
 * Ipv4Address 类的实现
 * ============================================================================
 */
Ipv4Address::Ipv4Address(const sockaddr_in& address)
{
}
Ipv4Address::Ipv4Address(uint32_t address, uint16_t port)
{
}

sockaddr* Ipv4Address::getAddr() const
{
}

socklen_t Ipv4Address::getAddrLen() const
{
}

std::ostream& Ipv4Address::insert(const std::ostream& out) const
{
}

IpAddress::ptr Ipv4Address::broadcastAddress(uint32_t prefix_len)
{
}

IpAddress::ptr Ipv4Address::networkAddress(uint32_t prefix_len)
{
}

IpAddress::ptr Ipv4Address::subnetMask(uint32_t prefix_len)
{
}

uint16_t Ipv4Address::getPort() const
{
}

void Ipv4Address::setPort(uint16_t port)
{
}

/**
 * ============================================================================
 * Ipv6Address 类的实现
 * ============================================================================
 */
Ipv6Address::Ipv6Address(const sockaddr_in6& address)
{
}

Ipv6Address::Ipv6Address(const uint8_t address[16], uint16_t port)
{
}

sockaddr* Ipv6Address::getAddr() const
{
}

socklen_t Ipv6Address::getAddrLen() const
{
}

std::ostream& Ipv6Address::insert(const std::ostream& out) const
{
}

IpAddress::ptr Ipv6Address::broadcastAddress(uint32_t prefix_len)
{
}

IpAddress::ptr Ipv6Address::networkAddress(uint32_t prefix_len)
{
}

IpAddress::ptr Ipv6Address::subnetMask(uint32_t prefix_len)
{
}

uint16_t Ipv6Address::getPort() const
{
}

void Ipv6Address::setPort(uint16_t port)
{
}

/**
 * ============================================================================
 * UnixAddress 类的实现
 * ============================================================================
 */
UnixAddress::UnixAddress()
{
}

UnixAddress::UnixAddress(const std::string& path)
{
}

std::string UnixAddress::getPath() const
{
}

sockaddr* UnixAddress::getAddr() const
{
}

socklen_t UnixAddress::getAddrLen() const
{
}

void UnixAddress::setAddrLen(socklen_t length)
{
}

std::ostream& UnixAddress::insert(const std::ostream& out) const
{
}

/**
 * ============================================================================
 * UnknownAddress 类的实现
 * ============================================================================
 */
UnknownAddress::UnknownAddress()
{
}
UnknownAddress::UnknownAddress(const sockaddr& address)
{
}

sockaddr* UnknownAddress::getAddr() const
{
}

socklen_t UnknownAddress::getAddrLen() const
{
}

std::ostream& UnknownAddress::insert(const std::ostream& out) const
{
}

} // namespace trycle
