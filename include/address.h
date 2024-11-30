#ifndef TRY_ADDRESS_H
#define TRY_ADDRESS_H

#include <arpa/inet.h>
#include <map>
#include <memory>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <vector>

namespace trycle
{

class IpAddress;

class Address
{
public:
    typedef std::shared_ptr<Address> ptr;

    /**
     * @brief 通过 sockaddr 指针创建 Address
     * @param {sockaddr*} addr sockaddr 指针
     * @param {socklen_t} addr_len sockaddr 的长度
     * @return {*} 返回和 sockaddr 相匹配的 Address，失败返回 nullptr
     */
    static Address::ptr Create(const sockaddr* addr, socklen_t addr_len);

    /**
     * @brief 通过 host 地址返回对应条件的所有 Address
     * @param {vector<Address::ptr>&} result 保存所有满足的 Address
     * @param {string&} host 域名、服务器名等，如，www.baidu.com[:80] (方括号表示可选内容)
     * @param {int} family 协议族（AF_INET, AF_INET6, AF_UNIX）
     * @param {int} type socket 类型 SOCK_STREAM, SOCK_DGRAM等
     * @param {int} protocol 协议，IPPROTO_TCP, IPROTO_UDP等
     * @return {*} 返回是否转换成功
     */
    static bool Lookup(std::vector<Address::ptr>& result, const std::string& host, int family = AF_INET, int type = 0, int protocol = 0);

    /**
     * @brief 通过 host 返回对应条件的任意 Address
     * @param {string&} host 域名、服务器名等，如，www.baidu.com[:80] (方括号表示可选内容)
     * @param {int} family 协议族（AF_INET, AF_INET6, AF_UNIX）
     * @param {int} type socket 类型 SOCK_STREAM, SOCK_DGRAM等
     * @param {int} protocol 协议，IPPROTO_TCP, IPROTO_UDP等
     * @return {*} 返回对应条件的 Address，失败返回 nullptr
     */
    static Address::ptr LookupAny(const std::string& host, int family = AF_INET, int type = 0, int protocol = 0);

    /**
     * @brief 通过 host 返回对应条件的任意 IpAddress
     * @param {string&} host 域名、服务器名等，如：www.baidu.com[:80] （方括号表示可选内容）
     * @param {int} family 协议族（AF_INET, AF_INET6, AF_UNIX）
     * @param {int} type socket 类型 SOCK_STREAM, SOCK_DGRAM 等
     * @param {int} protocol 协议，IPPROTO_TCP, IPROTO_UDP等
     * @return {*} 返回对应条件的任意 IpAddress，失败返回 nullptr
     */
    static std::shared_ptr<IpAddress> LookupAnyIpAddress(const std::string& host, int family = AF_INET, int type = 0, int protocol = 0);

    /**
     * @brief 获取本地所有网卡（网卡名、地址、子网掩码位数）
     * @param {multimap<std::string, std::pair<Address::ptr, uint32_t>>&} result 保存本地所有地址
     * @param {int} family 协议族（AF_INET, AF_INET6, AF_UNIX）
     * @return {*} 返回是否成功
     */
    static bool GetInterfaceAddress(std::multimap<std::string, std::pair<Address::ptr, uint32_t>>& result, int family = AF_INET);

    /**
     * @brief 获取指定网卡的地址和子网掩码位数
     * @param {vector<std::pair<Address::ptr, uint32_t>>&} result 保存指定网卡所有地址
     * @param {string&} iface 网卡名称
     * @param {int} family 协议族（AF_INET, AF_INET6, AF_UNIX）
     * @return {*} 返回是否成功
     */
    static bool GetInterfaceAddresses(std::vector<std::pair<Address::ptr, uint32_t>>& result, const std::string& iface, int family = AF_INET);

    virtual ~Address() {};

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
    /**
     * @brief 通过域名、ip、服务器名创建 IpAddress
     * @param {sockaddr*} addr 域名、ip或服务器名等，如：www.baidu.com
     * @param {socklen_t} addr_len ad
     * @return {*} 返回 IpAddress， 失败返回 nullptr
     */
    static IpAddress::ptr Create(const char* addr, uint16_t port = 0);

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

    /**
     * @brief 使用点分十进制地址创建 Ipv4Address
     * @param {char*} addr 点分十进制地址，如：192.168.1.1
     * @param {uint16_t} port 端口号
     * @return {*} 返回 Ipv4Address，失败返回 nullptr
     */
    static Ipv4Address::ptr Create(const char* addr, uint16_t port = 0);

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
    /**
     * @brief  通过 Ipv6 地址字符串构造 Ipv6Address
     * @param {char*} addr Ipv6 地址字符串
     * @param {uint16_t} port 端口号
     * @return {*} 返回 Ipv6Address, 失败返回 nullptr
     */
    static Ipv6Address::ptr Create(const char* addr, uint16_t port);

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