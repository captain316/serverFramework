#ifndef __CAPTAIN_ADDRESS_H__
#define __CAPTAIN_ADDRESS_H__

#include <memory>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <map>

namespace captain {

/* 
IPv4Address、IPv6Address 和 UnixAddress 继承自 IPAddress 而不是直接继承 Address 的原因在于代码的设计思想以及网络地址的层次结构。

IPv4 和 IPv6 地址在网络编程中通常有许多共性，例如获取和设置端口、获取广播地址、获取网络地址等。通过将这些共性抽象到 IPAddress 类中，可以使得 IPv4Address 和 IPv6Address 类只需要专注于处理与 IP 地址相关的操作，而不需要再重复定义这些共性操作。

同样地，Unix 域套接字地址在某些方面也有一些共性，但与 IP 地址的共性并不完全相同。通过将共性操作抽象到 Address 类中，可以确保 UnixAddress 类只需要专注于处理与 Unix 域套接字地址相关的操作。
 */

//Address：这是一个抽象基类，定义了一些公共的网络地址操作接口，如获取地址家族（Family）、获取地址长度、转化为字符串等。
class IPAddress;
class Address {
public:
    typedef std::shared_ptr<Address> ptr;

    static Address::ptr Create(const sockaddr* addr, socklen_t addrlen);
    //在给定的地址族、套接字类型和协议的约束下，解析给定的主机名，获取其对应的所有地址信息
    static bool Lookup(std::vector<Address::ptr>& result, const std::string& host,
            int family = AF_INET, int type = 0, int protocol = 0);
    //查找给定主机名的任意可用地址
    static Address::ptr LookupAny(const std::string& host,
            int family = AF_INET, int type = 0, int protocol = 0);
    //查找给定主机名的任意可用 IP 地址
    static std::shared_ptr<IPAddress> LookupAnyIPAddress(const std::string& host,
            int family = AF_INET, int type = 0, int protocol = 0);


    /*GetInterfaceAddresses
      获取本机上【所有】网络接口的地址信息，并将其存储在 result 参数提供的 std::multimap 容器中。
      对于每个网络接口，函数将其名称、地址以及接口索引存储在 result 容器中。

      result: 一个 std::multimap，用于存储网络接口地址信息。
     */
    static bool GetInterfaceAddresses(std::multimap<std::string
                    ,std::pair<Address::ptr, uint32_t> >& result,
                    int family = AF_INET);
    //获取【特定】名称的网络接口的地址信息
    static bool GetInterfaceAddresses(std::vector<std::pair<Address::ptr, uint32_t> >&result
                    ,const std::string& iface, int family = AF_INET);

    virtual ~Address() {}

    int getFamily() const; //区分socket类型

    virtual const sockaddr* getAddr() const = 0;
    virtual sockaddr* getAddr() = 0;
    virtual socklen_t getAddrLen() const = 0;

    virtual std::ostream& insert(std::ostream& os) const = 0;
    std::string toString() const;

    bool operator<(const Address& rhs) const;
    bool operator==(const Address& rhs) const;
    bool operator!=(const Address& rhs) const;
};

//IPAddress：继承自 Address，表示 IP 地址，包括 IPv4 和 IPv6。它定义了一些操作接口，如获取端口、设置端口、获取广播地址、网络地址等。
class IPAddress : public Address {
public:
    typedef std::shared_ptr<IPAddress> ptr;

    static IPAddress::ptr Create(const char* address, uint16_t port = 0);

    virtual IPAddress::ptr broadcastAddress(uint32_t prefix_len) = 0; //广播地址
    virtual IPAddress::ptr networdAddress(uint32_t prefix_len) = 0; //网络地址
    virtual IPAddress::ptr subnetMask(uint32_t prefix_len) = 0; //子网地址/掩码

    virtual uint32_t getPort() const = 0;
    virtual void setPort(uint16_t v) = 0;
};
//IPv4Address：继承自 IPAddress，表示 IPv4 地址，包括获取和设置 IPv4 地址及端口。
class IPv4Address : public IPAddress {
public:
    typedef std::shared_ptr<IPv4Address> ptr;

    static IPv4Address::ptr Create(const char* address, uint16_t port = 0);

    IPv4Address(const sockaddr_in& address);
    IPv4Address(uint32_t address = INADDR_ANY, uint16_t port = 0);

    const sockaddr* getAddr() const override;
    sockaddr* getAddr() override;
    socklen_t getAddrLen() const override;
    std::ostream& insert(std::ostream& os) const override;

    IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;
    IPAddress::ptr networdAddress(uint32_t prefix_len) override;
    IPAddress::ptr subnetMask(uint32_t prefix_len) override;
    uint32_t getPort() const override;
    void setPort(uint16_t v) override;
private:
    sockaddr_in m_addr;
};

//IPv6Address：继承自 IPAddress，表示 IPv6 地址，包括获取和设置 IPv6 地址及端口。
class IPv6Address : public IPAddress {
public:
    typedef std::shared_ptr<IPv6Address> ptr;
    static IPv6Address::ptr Create(const char* address, uint16_t port = 0);

    IPv6Address();
    IPv6Address(const sockaddr_in6& address);
    IPv6Address(const uint8_t address[16], uint16_t port = 0);

    const sockaddr* getAddr() const override;
    sockaddr* getAddr() override;
    socklen_t getAddrLen() const override;
    std::ostream& insert(std::ostream& os) const override;

    IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;
    IPAddress::ptr networdAddress(uint32_t prefix_len) override;
    IPAddress::ptr subnetMask(uint32_t prefix_len) override;
    uint32_t getPort() const override;
    void setPort(uint16_t v) override;
private:
    sockaddr_in6 m_addr;
};

//UnixAddress：继承自 Address，表示 Unix 域套接字地址，包括获取和设置地址。
class UnixAddress : public Address {
public:
    typedef std::shared_ptr<UnixAddress> ptr;
    UnixAddress();
    UnixAddress(const std::string& path);

    const sockaddr* getAddr() const override;
    sockaddr* getAddr() override;
    socklen_t getAddrLen() const override;
    void setAddrLen(uint32_t v);
    std::ostream& insert(std::ostream& os) const override;
private:
    sockaddr_un m_addr;
    socklen_t m_length;
};

//UnknownAddress：继承自 Address，表示未知类型的地址。
class UnknownAddress : public Address {
public:
    typedef std::shared_ptr<UnknownAddress> ptr;
    UnknownAddress(int family);
    UnknownAddress(const sockaddr& addr);
    const sockaddr* getAddr() const override;
    sockaddr* getAddr() override;
    socklen_t getAddrLen() const override;
    std::ostream& insert(std::ostream& os) const override;
private:
    sockaddr m_addr;
};

std::ostream& operator<<(std::ostream& os, const Address& addr);

}

#endif
