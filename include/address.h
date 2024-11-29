#ifndef TRY_ADDRESS_H
#define TRY_ADDRESS_H

#include <arpa/inet.h>
#include <memory>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

namespace trycle
{

class Address
{
public:
    typedef std::shared_ptr<Address> ptr;

    virtual ~Address();

    int getFamily() const;
    virtual sockaddr* getAddr() const    = 0;
    virtual socklen_t getAddrLen() const = 0;

    std::string toString() const;
    virtual std::ostream& insert(std::ostream& out) const = 0;

    bool operator<(const Address& rhs) const;
    bool operator==(const Address& rhs) const;
    bool operator!=(const Address& rhs) const;
};

class IpAddress : public Address
{
public:
    typedef std::shared_ptr<IpAddress> ptr;

    virtual IpAddress::ptr broadcastAddress(uint32_t prefix_len) = 0;
    virtual IpAddress::ptr networkAddress(uint32_t refix_len)    = 0;
    virtual IpAddress::ptr subnetMask(uint32_t prefix_len)       = 0;

    virtual uint16_t getPort() const                             = 0;
    virtual void setPort(uint16_t port)                          = 0;

private:
    sockaddr* m_addr;
    uint16_t m_port;
};

class Ipv4Address : public IpAddress
{
public:
    typedef std::shared_ptr<Ipv4Address> ptr;

    Ipv4Address();
    Ipv4Address(const sockaddr_in& address);
    Ipv4Address(uint32_t address = INADDR_ANY, uint16_t port = 0);

    sockaddr* getAddr() const override;
    socklen_t getAddrLen() const override;

    std::ostream& insert(std::ostream& out) const override;

    IpAddress::ptr broadcastAddress(uint32_t prefix_len) override;
    IpAddress::ptr networkAddress(uint32_t prefix_len) override;
    IpAddress::ptr subnetMask(uint32_t prefix_len) override;

    uint16_t getPort() const override;
    void setPort(uint16_t port) override;

private:
    sockaddr_in m_addr;
};

class Ipv6Address : public IpAddress
{
public:
    typedef std::shared_ptr<Ipv6Address> ptr;

    Ipv6Address();
    Ipv6Address(const sockaddr_in6& address);
    Ipv6Address(const uint8_t address[16], uint16_t port = 0);

    sockaddr* getAddr() const override;
    socklen_t getAddrLen() const override;

    std::ostream& insert(std::ostream& out) const override;

    IpAddress::ptr broadcastAddress(uint32_t prefix_len) override;
    IpAddress::ptr networkAddress(uint32_t prefix_len) override;
    IpAddress::ptr subnetMask(uint32_t prefix_len) override;

    uint16_t getPort() const override;
    void setPort(uint16_t port) override;

private:
    sockaddr_in6 m_addr;
};

class UnixAddress : public Address
{
public:
    typedef std::shared_ptr<UnixAddress> ptr;

    UnixAddress();
    /**
     * @brief 通过路径构造 UnixAddress
     * @param path UnixSocket 路径（长度小于 UNIX_PATH_MAX ）
     */
    UnixAddress(const std::string& path);

    std::string getPath() const;

    sockaddr* getAddr() const override;
    socklen_t getAddrLen() const override;
    void setAddrLen(socklen_t length);

    std::ostream& insert(std::ostream& out) const override;

private:
    sockaddr_un m_addr;
    socklen_t m_socklen;
};

class UnknownAddress : public Address
{
public:
    UnknownAddress(int family);
    UnknownAddress(const sockaddr& address);

    sockaddr* getAddr() const override;
    socklen_t getAddrLen() const override;

    std::ostream& insert(std::ostream& out) const override;

private:
    sockaddr m_addr;
};

std::ostream& operator<<(std::ostream& out, const Address& operand);

} // namespace trycle

#endif // TRY_ADDRESS_H