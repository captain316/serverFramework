#ifndef __CAPTAIN_SOCKET_H__
#define __CAPTAIN_SOCKET_H__

#include <memory>
#include "address.h"
#include "noncopyable.h"

namespace captain {

class Socket : public std::enable_shared_from_this<Socket>, Noncopyable {
public:
    typedef std::shared_ptr<Socket> ptr;
    typedef std::weak_ptr<Socket> weak_ptr;

    enum Type {
        TCP = SOCK_STREAM,
        UDP = SOCK_DGRAM
    };

    enum Family {
        IPv4 = AF_INET,
        IPv6 = AF_INET6,
        UNIX = AF_UNIX,
    };

    static Socket::ptr CreateTCP(captain::Address::ptr address);
    static Socket::ptr CreateUDP(captain::Address::ptr address);

    static Socket::ptr CreateTCPSocket();
    static Socket::ptr CreateUDPSocket();

    static Socket::ptr CreateTCPSocket6();
    static Socket::ptr CreateUDPSocket6();

    static Socket::ptr CreateUnixTCPSocket();
    static Socket::ptr CreateUnixUDPSocket();

    Socket(int family, int type, int protocol = 0);
    ~Socket();

    
    int64_t getSendTimeout();  //获取发送超时时间
    void setSendTimeout(int64_t v); //设置发送超时时间

    int64_t getRecvTimeout(); //获取接收超时时间
    void setRecvTimeout(int64_t v); //设置接收超时时间
    
    //getOption 获取套接字选项的通用函数
    bool getOption(int level, int option, void* result, size_t* len);
    template<class T>
    bool getOption(int level, int option, T& result) {
        size_t length = sizeof(T);
        return getOption(level, option, &result, &length);
    }

    bool setOption(int level, int option, const void* result, size_t len);
    template<class T>
    bool setOption(int level, int option, const T& value) {
        return setOption(level, option, &value, sizeof(T));
    }

    Socket::ptr accept();

    bool bind(const Address::ptr addr); //将套接字绑定到指定的本地地址（Address）
    bool connect(const Address::ptr addr, uint64_t timeout_ms = -1); //客户端连接到远程服务器。
    bool listen(int backlog = SOMAXCONN); //将套接字设置为监听模式，用于服务器端。它指示套接字开始接受传入的连接请求。
    bool close();

    int send(const void* buffer, size_t length, int flags = 0); //发送一块连续的内存区域的数据。
    int send(const iovec* buffers, size_t length, int flags = 0); //发送多块连续的内存区域的数据。
    int sendTo(const void* buffer, size_t length, const Address::ptr to, int flags = 0); //通过套接字发送数据到指定的远程地址。
    int sendTo(const iovec* buffers, size_t length, const Address::ptr to, int flags = 0); //通过套接字发送多块连续的数据到指定的远程地址。

    int recv(void* buffer, size_t length, int flags = 0);
    int recv(iovec* buffers, size_t length, int flags = 0); //从套接字接收数据并存储到指定的多块连续内存区域中。
    int recvFrom(void* buffer, size_t length, Address::ptr from, int flags = 0); //从套接字接收数据并存储到指定的连续内存区域中，并返回数据来源的地址信息。
    int recvFrom(iovec* buffers, size_t length, Address::ptr from, int flags = 0); //用于从套接字接收数据并存储到指定的多块连续内存区域中，并返回数据来源的地址信息。

    Address::ptr getRemoteAddress(); //用于获取套接字连接的远程地址（对端地址）。
    Address::ptr getLocalAddress(); //用于获取套接字的本地地址。 可以用于获取本地主机的IP地址和端口号等信息。

    int getFamily() const { return m_family;} //返回套接字的地址族，例如 AF_INET 表示 IPv4 地址族，AF_INET6 表示 IPv6 地址族，AF_UNIX 表示 Unix 域套接字。
    int getType() const { return m_type;} //返回套接字的类型，例如 SOCK_STREAM 表示 TCP 套接字，SOCK_DGRAM 表示 UDP 套接字。
    int getProtocol() const { return m_protocol;} //返回套接字的协议

    bool isConnected() const { return m_isConnected;}
    bool isValid() const; //检查套接字是否有效。
    int getError(); //获取套接字的错误码。

    //将套接字的信息以文本形式输出到给定的输出流 os 中。它通常用于调试和日志记录，以便查看套接字的状态和属性。
    std::ostream& dump(std::ostream& os) const;
    int getSocket() const { return m_sock;} //返回套接字的句柄（文件描述符）

    bool cancelRead(); //取消套接字上的读操作。
    bool cancelWrite();
    bool cancelAccept();
    bool cancelAll(); //取消套接字上的所有操作。
private:
    //初始化套接字和创建新套接字。
    void initSock();
    void newSock();
    bool init(int sock);
private:
    int m_sock; //套接字描述符，用于标识套接字。
    int m_family; //套接字的地址族（IPv4、IPv6、UNIX 等）。
    int m_type; //套接字的类型（TCP 或 UDP）。
    int m_protocol; //套接字的协议。
    bool m_isConnected;

    Address::ptr m_localAddress; // 存储本地地址信息的地址对象。
    Address::ptr m_remoteAddress; //存储远程地址信息的地址对象。
};

std::ostream& operator<<(std::ostream& os, const Socket& addr);

}

#endif
