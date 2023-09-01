#include "include/address.h"
#include "include/log.h"
#include <sstream>
#include <netdb.h>
#include <ifaddrs.h>
#include <stddef.h>

#include "include/endian.h"

namespace captain {

static captain::Logger::ptr g_logger = CAPTAIN_LOG_NAME("system");

/* CreateMask  根据传入的位数，生成一个二进制掩码
假设我们使用 CreateMask<uint8_t>(3)。首先，我们知道 sizeof(uint8_t) 是 1，所以 sizeof(uint8_t) * 8 是 8，
即一个 uint8_t 数据类型有 8 位。然后我们有传入的 bits 参数是 3，表示我们想要生成一个 3 位的掩码。

现在，我们按照函数的计算步骤来计算：

1、sizeof(T) * 8：1 * 8 = 8。
2、sizeof(T) * 8 - bits：8 - 3 = 5。
3、1 << (sizeof(T) * 8 - bits)：1 << 5 = 32，即二进制 00100000。
4、(1 << (sizeof(T) * 8 - bits)) - 1：32 - 1 = 31，即二进制 00011111。
所以，CreateMask<uint8_t>(3) 将返回值 31，即二进制 00011111。
 */
template<class T>
static T CreateMask(uint32_t bits) {
    return (1 << (sizeof(T) * 8 - bits)) - 1;
}

//计算一个整数中二进制表示中的 1 的个数。
template<class T>
static uint32_t CountBytes(T value) {
    uint32_t result = 0;
    for(; value; ++result) {
        value &= value - 1;
    }
    return result;
}
//查找指定主机名的任意可用地址，并返回一个 Address::ptr 智能指针，指向找到的第一个地址。如果查找失败，它会返回一个空指针。
Address::ptr Address::LookupAny(const std::string& host,
                                int family, int type, int protocol) {
    std::vector<Address::ptr> result;
    if(Lookup(result, host, family, type, protocol)) {
        return result[0];
    }
    return nullptr;
}

//查找指定主机名的任意可用 IP 地址，并返回一个指向第一个找到的 IP 地址的智能指针。如果找不到 IP 地址，将返回空指针。
IPAddress::ptr Address::LookupAnyIPAddress(const std::string& host,
                                int family, int type, int protocol) {
    std::vector<Address::ptr> result;
    if(Lookup(result, host, family, type, protocol)) {
        for(auto& i : result) {
            std::cout << i->toString() << std::endl;
        }
        for(auto& i : result) {
            IPAddress::ptr v = std::dynamic_pointer_cast<IPAddress>(i);
            if(v) {
                return v;
            }
        }
    }
    return nullptr;
}

//在给定的地址族、套接字类型和协议的约束下，解析给定的主机名，获取其对应的所有地址信息
bool Address::Lookup(std::vector<Address::ptr>& result, const std::string& host,
                     int family, int type, int protocol) {
    //创建了 addrinfo 结构体的实例 hints，并对其成员进行设置，以指定主机名解析的一些条件和选项。
    addrinfo hints, *results, *next;
    hints.ai_flags = 0;
    hints.ai_family = family;
    hints.ai_socktype = type;
    hints.ai_protocol = protocol;
    hints.ai_addrlen = 0;
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    std::string node;
    const char* service = NULL;

    //检查 ipv6address serivce ;检查主机名是否符合 IPv6 地址的格式，如果是的话，提取出其中的节点部分并在需要的情况下提取出服务端口部分。
    if(!host.empty() && host[0] == '[') { //判断语句检查主机名是否非空且以 [ 开头，这是 IPv6 地址的特点。
        // endipv6 将指向 ] 字符在主机名中的位置
        const char* endipv6 = (const char*)memchr(host.c_str() + 1, ']', host.size() - 1);
        if(endipv6) { //检查是否找到了 ] 字符。
            //TODO check out of range
            if(*(endipv6 + 1) == ':') { //如果 ] 字符后面的字符是 :，说明主机名中还包含服务端口信息。
                service = endipv6 + 2; //将 service 指针指向 ] 字符之后的字符，即服务端口信息的起始位置。
            }
            //从主机名中提取出 IPv6 地址的节点部分。
            node = host.substr(1, endipv6 - host.c_str() - 1);
        }
    }

    //检查 node serivce
    //检查主机名中是否包含节点和服务端口信息，如果包含，则提取出这些信息，并在需要的情况下将服务端口信息的起始位置存储在 service 指针中。
    if(node.empty()) {
        service = (const char*)memchr(host.c_str(), ':', host.size());
        if(service) {
            if(!memchr(service + 1, ':', host.c_str() + host.size() - service - 1)) {
                node = host.substr(0, service - host.c_str());
                ++service;
            }
        }
    }
    //以下代码块负责使用提取出的主机名和服务端口信息执行地址解析操作，并将解析结果存储在 results 中。
    if(node.empty()) {
        node = host; //如果没有提取出节点信息，那么将整个主机名赋值给 node 变量。
    }
    //使用 getaddrinfo 函数执行地址解析操作。它接受主机名、服务端口信息、解析选项等作为参数，并将解析结果存储在 results 链表中。
    int error = getaddrinfo(node.c_str(), service, &hints, &results);
    if(error) {
        CAPTAIN_LOG_ERROR(g_logger) << "Address::Lookup getaddress(" << host << ", "
            << family << ", " << type << ") err=" << error << " errstr="
            << strerror(errno);
        return false;
    }
    //将指向解析结果链表的指针 next 设置为链表的头部。
    next = results;
    while(next) { //循环遍历解析结果链表
        //将每个解析结果的地址信息创建成相应的 Address 对象，并存储在 result 向量中。
        result.push_back(Create(next->ai_addr, (socklen_t)next->ai_addrlen));
        //CAPTAIN_LOG_INFO(g_logger) << ((sockaddr_in*)next->ai_addr)->sin_addr.s_addr;
        next = next->ai_next; //将 next 指针指向下一个解析结果节点。
    }

    freeaddrinfo(results); //释放掉之前调用 getaddrinfo 时分配的内存。
    return true;    //解析成功
}
//getifaddrs：可以拿到网卡相关的地址信息
/* 
获取系统上所有网络接口的地址信息，并将其存储在提供的 result 参数中的 std::multimap 容器中。
对于每个接口，函数将其名称、地址以及接口的前缀长度存储在 result 容器中。
 */
bool Address::GetInterfaceAddresses(std::multimap<std::string
                    ,std::pair<Address::ptr, uint32_t> >& result,
                    int family) {
    //获取系统上所有网络接口的信息
    struct ifaddrs *next, *results;
    if(getifaddrs(&results) != 0) {
        CAPTAIN_LOG_ERROR(g_logger) << "Address::GetInterfaceAddresses getifaddrs "
            " err=" << errno << " errstr=" << strerror(errno);
        return false;
    }

    try {
        //遍历获取的接口信息，每次迭代处理一个接口。
        for(next = results; next; next = next->ifa_next) {
            Address::ptr addr; //存储当前接口的地址信息
            uint32_t prefix_len = ~0u; //初始化子网前缀长度为一个无效值。
            //检查接口的地址类型是否与指定的地址类型匹配，如果不匹配，跳过当前迭代。
            if(family != AF_UNSPEC && family != next->ifa_addr->sa_family) {
                continue;
            }
            //根据接口的地址类型进行不同的处理。
            switch(next->ifa_addr->sa_family) {
                //对于 AF_INET 类型（IPv4地址），使用 Create 函数创建地址对象，获取子网掩码，并通过 CountBytes 函数计算子网前缀长度。
                case AF_INET:
                    {
                        addr = Create(next->ifa_addr, sizeof(sockaddr_in));
                        uint32_t netmask = ((sockaddr_in*)next->ifa_netmask)->sin_addr.s_addr;
                        prefix_len = CountBytes(netmask);
                    }
                    break;
                //对于 AF_INET6 类型（IPv6地址），同样使用 Create 函数创建地址对象，获取子网掩码，并通过循环计算子网前缀长度。
                case AF_INET6:
                    {
                        addr = Create(next->ifa_addr, sizeof(sockaddr_in6));
                        in6_addr& netmask = ((sockaddr_in6*)next->ifa_netmask)->sin6_addr;
                        prefix_len = 0;
                        for(int i = 0; i < 16; ++i) {
                            prefix_len += CountBytes(netmask.s6_addr[i]);
                        }
                    }
                    break;
                default:
                    break;
            }
            //如果成功创建了地址对象，则将该地址对象以及子网前缀长度插入到 result 容器中，使用接口名作为键。
            if(addr) {
                result.insert(std::make_pair(next->ifa_name,
                            std::make_pair(addr, prefix_len)));
            }
        }
    } catch (...) {
        CAPTAIN_LOG_ERROR(g_logger) << "Address::GetInterfaceAddresses exception";
        freeifaddrs(results);
        return false;
    }
    freeifaddrs(results);
    return true;
}

//获取特定接口的网络地址信息。
bool Address::GetInterfaceAddresses(std::vector<std::pair<Address::ptr, uint32_t> >&result
                    ,const std::string& iface, int family) {
    if(iface.empty() || iface == "*") {//如果传入的接口名为空或为通配符 *，则表示获取所有接口的地址信息。
        if(family == AF_INET || family == AF_UNSPEC) {
            result.push_back(std::make_pair(Address::ptr(new IPv4Address()), 0u));
        }
        if(family == AF_INET6 || family == AF_UNSPEC) {
            result.push_back(std::make_pair(Address::ptr(new IPv6Address()), 0u));
        }
        return true;
    }
    //如果不是获取所有接口地址信息，那么调用 GetInterfaceAddresses 函数获取系统的接口地址信息，并将结果存储在一个临时的 results 容器中。
    std::multimap<std::string
          ,std::pair<Address::ptr, uint32_t> > results;

    if(!GetInterfaceAddresses(results, family)) {
        return false;
    }
    //使用 equal_range 函数在 results 容器中找到指定接口名的范围。
    auto its = results.equal_range(iface);
    //遍历找到的范围，将每个接口的地址信息（Address::ptr）和子网前缀长度插入到 result 容器中。
    for(; its.first != its.second; ++its.first) {
        result.push_back(its.first->second);
    }
    return true;
}

//获取地址的家族类型（Family Type）
int Address::getFamily() const {
    //sa_family 是套接字地址结构中的成员，表示家族类型，
    //如 AF_INET 表示 IPv4 地址家族，AF_INET6 表示 IPv6 地址家族等。
    return getAddr()->sa_family;
}

//将地址转换为字符串形式。
std::string Address::toString() const {
    std::stringstream ss;
    insert(ss);
    return ss.str();
}

//根据不同的地址族类型，创建相应类型的 Address 实例，用于对不同类型的地址进行封装。
Address::ptr Address::Create(const sockaddr* addr, socklen_t addrlen) {
    if(addr == nullptr) {
        return nullptr;
    }

    Address::ptr result; //创建一个 Address::ptr 实例，即 Address 类的智能指针。
    switch(addr->sa_family) { //判断地址族的类型
        case AF_INET:
            //reset 是智能指针的一个成员函数，用于将智能指针重新指向一个新的对象或者将其置为空。
            result.reset(new IPv4Address(*(const sockaddr_in*)addr));
            break;
        case AF_INET6:
            result.reset(new IPv6Address(*(const sockaddr_in6*)addr));
            break;
        default:
            //未知的地址族，创建一个 UnknownAddress 类的实例，并将其赋值给 result。
            result.reset(new UnknownAddress(*addr));
            break;
    }
    return result;
}

//比较两个地址的大小关系
bool Address::operator<(const Address& rhs) const {
    //首先获取两个地址中长度较小的那个长度，然后使用 memcmp() 函数比较这部分内容。
    socklen_t minlen = std::min(getAddrLen(), rhs.getAddrLen());
    int result = memcmp(getAddr(), rhs.getAddr(), minlen);
    if(result < 0) {
        return true;
    } else if(result > 0) {
        return false;
    } else if(getAddrLen() < rhs.getAddrLen()) {
        return true;
    }
    return false;
}

//比较两个地址是否相等 如果内容相等且长度相等，则表示两个地址相等
bool Address::operator==(const Address& rhs) const {
    //memcmp表示比较当前对象的地址数据和另一个对象的地址数据的内容是否相等，比较的字节数是 getAddrLen()，即地址的长度。
    return getAddrLen() == rhs.getAddrLen()
        && memcmp(getAddr(), rhs.getAddr(), getAddrLen()) == 0;
}

bool Address::operator!=(const Address& rhs) const {
    return !(*this == rhs);
}

//这个函数的目的是将不同形式的地址字符串解析成为统一的 IPAddress 实例。
IPAddress::ptr IPAddress::Create(const char* address, uint16_t port) {
    addrinfo hints, *results;
    memset(&hints, 0, sizeof(addrinfo));
    //进行一个数值地址的解析（AI_NUMERICHOST 标志），并且不限制地址族（AF_UNSPEC）。
    hints.ai_flags = AI_NUMERICHOST;
    hints.ai_family = AF_UNSPEC;
    //调用 getaddrinfo 函数，传入要解析的地址字符串和端口为 NULL，以及之前创建的 hints 结构体。
    //这个函数会尝试将地址字符串解析为一个或多个地址结构体，解析的结果会存储在一个 addrinfo 结构体链表 results 中。
    int error = getaddrinfo(address, NULL, &hints, &results);//如果解析成功（getaddrinfo 返回 0）
    if(error) {
        CAPTAIN_LOG_ERROR(g_logger) << "IPAddress::Create(" << address
            << ", " << port << ") error=" << error
            << " errno=" << errno << " errstr=" << strerror(errno);
        return nullptr;
    }

    try {
        //尝试使用 Address::Create 函数将解析得到的地址结构体转换为 Address 类的实例。
        //使用了 std::dynamic_pointer_cast 来进行类型转换，将 Address 实例转换为 IPAddress 实例。
        IPAddress::ptr result = std::dynamic_pointer_cast<IPAddress>(
                Address::Create(results->ai_addr, (socklen_t)results->ai_addrlen));
        if(result) { //如果转换成功，则设置实例的端口为传入的端口。
            result->setPort(port);
        }
        //释放通过 getaddrinfo 分配的内存，调用 freeaddrinfo 函数。
        freeaddrinfo(results);
        return result;
    } catch (...) {
        freeaddrinfo(results);
        return nullptr;
    }
}

//解析 IP 地址字符串并设置端口号，创建了一个 IPv4Address 对象。把文本类型地址转换为ipv4
IPv4Address::ptr IPv4Address::Create(const char* address, uint16_t port) {
    IPv4Address::ptr rt(new IPv4Address);
    rt->m_addr.sin_port = byteswapOnLittleEndian(port);
    int result = inet_pton(AF_INET, address, &rt->m_addr.sin_addr);
    if(result <= 0) { //返回 > 0 表示成功，0 表示格式无效，< 0 表示错误
        CAPTAIN_LOG_ERROR(g_logger) << "IPv4Address::Create(" << address << ", "
                << port << ") rt=" << result << " errno=" << errno
                << " errstr=" << strerror(errno);
        return nullptr;
    }
    return rt;
}

IPv4Address::IPv4Address(const sockaddr_in& address) {
    m_addr = address;
}

//创建一个 IPv4Address 对象并正确初始化其内部的 sockaddr_in 结构体
IPv4Address::IPv4Address(uint32_t address, uint16_t port) {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin_family = AF_INET;
    m_addr.sin_port = byteswapOnLittleEndian(port);
    m_addr.sin_addr.s_addr = byteswapOnLittleEndian(address);
}

//返回一个指向 m_addr 的 sockaddr 指针，以便其他函数可以使用该指针来访问地址结构体。
sockaddr* IPv4Address::getAddr() {
    return (sockaddr*)&m_addr;
}

//针对 const 对象的，保证不会修改成员变量。
const sockaddr* IPv4Address::getAddr() const {
    return (sockaddr*)&m_addr;
}

//返回地址结构体的长度
socklen_t IPv4Address::getAddrLen() const {
    return sizeof(m_addr);
}

// 将IPv4 地址和端口号以 "x.x.x.x:y" 的形式插入到输出流中，其中 x.x.x.x 表示IPv4地址，y 表示端口号。
std::ostream& IPv4Address::insert(std::ostream& os) const {
    uint32_t addr = byteswapOnLittleEndian(m_addr.sin_addr.s_addr);
    os << ((addr >> 24) & 0xff) << "."
       << ((addr >> 16) & 0xff) << "."
       << ((addr >> 8) & 0xff) << "."
       << (addr & 0xff);
    os << ":" << byteswapOnLittleEndian(m_addr.sin_port);
    return os;
}

//计算给定前缀长度的 IPv4 广播地址，并返回一个包含该广播地址的新的 IPv4 地址对象的智能指针。
IPAddress::ptr IPv4Address::broadcastAddress(uint32_t prefix_len) {
    //检查给定的前缀长度是否合法。IPv4 地址的前缀长度范围是从 0 到 32，所以如果前缀长度超过 32，就返回一个空指针。
    if(prefix_len > 32) {
        return nullptr;
    }
    //创建了一个与当前 IPv4 地址相同的新的 sockaddr_in 结构体 baddr，以备后续修改。
    sockaddr_in baddr(m_addr);
    //通过位或操作，将广播地址的主机部分设置为前缀掩码对应的位模式。这样可以获得给定前缀长度的广播地址。
    baddr.sin_addr.s_addr |= byteswapOnLittleEndian(
            CreateMask<uint32_t>(prefix_len));
    //创建一个新的 IPv4 地址对象，传入修改后的 sockaddr_in 结构体作为参数，并返回该地址对象的智能指针。
    return IPv4Address::ptr(new IPv4Address(baddr));
}

//网段  生成指定前缀长度的网络地址，使得网络位之外的位均被置零。
IPAddress::ptr IPv4Address::networdAddress(uint32_t prefix_len) {
    if(prefix_len > 32) {
        return nullptr;
    }

    sockaddr_in baddr(m_addr);
    baddr.sin_addr.s_addr &= byteswapOnLittleEndian(
            CreateMask<uint32_t>(prefix_len));
    return IPv4Address::ptr(new IPv4Address(baddr));
}

//生成指定前缀长度的子网掩码，使得网络位之外的位均为1，网络位为0。
IPAddress::ptr IPv4Address::subnetMask(uint32_t prefix_len) {
    sockaddr_in subnet;
    memset(&subnet, 0, sizeof(subnet));
    subnet.sin_family = AF_INET;
    subnet.sin_addr.s_addr = ~byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len));
    return IPv4Address::ptr(new IPv4Address(subnet));
}

//getPort 函数用于获取当前 IPv4 地址的端口号，
//而 setPort 函数用于设置当前 IPv4 地址的端口号。在设置端口号时，函数会确保正确的字节顺序。
uint32_t IPv4Address::getPort() const {
    return byteswapOnLittleEndian(m_addr.sin_port);
}

void IPv4Address::setPort(uint16_t v) {
    m_addr.sin_port = byteswapOnLittleEndian(v);
}

//将文本形式的 IPv6 地址转换为程序内部的 IPv6 地址数据结构
IPv6Address::ptr IPv6Address::Create(const char* address, uint16_t port) {
    IPv6Address::ptr rt(new IPv6Address);
    rt->m_addr.sin6_port = byteswapOnLittleEndian(port);
    int result = inet_pton(AF_INET6, address, &rt->m_addr.sin6_addr);
    if(result <= 0) {
        CAPTAIN_LOG_ERROR(g_logger) << "IPv6Address::Create(" << address << ", "
                << port << ") rt=" << result << " errno=" << errno
                << " errstr=" << strerror(errno);
        return nullptr;
    }
    return rt;
}

IPv6Address::IPv6Address() {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin6_family = AF_INET6;
}

IPv6Address::IPv6Address(const sockaddr_in6& address) {
    m_addr = address;
}

//创建一个包含指定 IPv6 地址和端口号的 IPv6Address 对象
IPv6Address::IPv6Address(const uint8_t address[16], uint16_t port) {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin6_family = AF_INET6;
    m_addr.sin6_port = byteswapOnLittleEndian(port);
    memcpy(&m_addr.sin6_addr.s6_addr, address, 16);
}

sockaddr* IPv6Address::getAddr() {
    return (sockaddr*)&m_addr;
}

const sockaddr* IPv6Address::getAddr() const {
    return (sockaddr*)&m_addr;
}

socklen_t IPv6Address::getAddrLen() const {
    return sizeof(m_addr);
}

/* 将 IPv6 地址以特定的格式插入到输出流中，格式为 "[addr]:port"，其中 "addr" 代表 IPv6 地址，"port" 代表端口号。

   如果 IPv6 地址为 2001:0db8:85a3:0000:0000:8a2e:0370:7334，端口号为 8080，
   那么调用这个函数后的输出将会是：[2001:db8:85a3::8a2e:370:7334]:8080

   在输出中，IPv6 地址使用了标准的缩写格式，即连续的 0 分段会被 "::" 替代，以减少输出的长度。
   端口号则直接跟在地址后面，用冒号 ":" 分隔。
*/
std::ostream& IPv6Address::insert(std::ostream& os) const {
    os << "[";
    uint16_t* addr = (uint16_t*)m_addr.sin6_addr.s6_addr;
    bool used_zeros = false;
    for(size_t i = 0; i < 8; ++i) {
        if(addr[i] == 0 && !used_zeros) {
            continue;
        }
        if(i && addr[i - 1] == 0 && !used_zeros) {
            os << ":";
            used_zeros = true;
        }
        if(i) {
            os << ":";
        }
        os << std::hex << (int)byteswapOnLittleEndian(addr[i]) << std::dec;
    }

    if(!used_zeros && addr[7] == 0) {
        os << "::";
    }

    os << "]:" << byteswapOnLittleEndian(m_addr.sin6_port);
    return os;
}

//生成指定前缀长度的 IPv6 地址的广播地址，使得指定前缀长度之后的所有位均为 1。
IPAddress::ptr IPv6Address::broadcastAddress(uint32_t prefix_len) {
    sockaddr_in6 baddr(m_addr);
    baddr.sin6_addr.s6_addr[prefix_len / 8] |=
        CreateMask<uint8_t>(prefix_len % 8);
    for(int i = prefix_len / 8 + 1; i < 16; ++i) {
        baddr.sin6_addr.s6_addr[i] = 0xff;
    }
    return IPv6Address::ptr(new IPv6Address(baddr));
}

//生成一个具有特定前缀长度的 IPv6 子网地址，其中前缀位保持不变，而后缀位被清零。这样可以确保该子网地址包含了所有与之具有相同前缀的其他 IPv6 地址。
IPAddress::ptr IPv6Address::networdAddress(uint32_t prefix_len) {
    sockaddr_in6 baddr(m_addr);
    baddr.sin6_addr.s6_addr[prefix_len / 8] &=
        CreateMask<uint8_t>(prefix_len % 8);
    for(int i = prefix_len / 8 + 1; i < 16; ++i) {
        baddr.sin6_addr.s6_addr[i] = 0x00;
    }
    return IPv6Address::ptr(new IPv6Address(baddr));
}

//生成一个具有指定前缀长度的 IPv6 子网掩码 子网掩码用于在 IPv6 地址中划分出网络部分和主机部分。
IPAddress::ptr IPv6Address::subnetMask(uint32_t prefix_len) {
    sockaddr_in6 subnet;
    memset(&subnet, 0, sizeof(subnet));
    subnet.sin6_family = AF_INET6;
    subnet.sin6_addr.s6_addr[prefix_len /8] =
        ~CreateMask<uint8_t>(prefix_len % 8);

    for(uint32_t i = 0; i < prefix_len / 8; ++i) {
        subnet.sin6_addr.s6_addr[i] = 0xff;
    }
    return IPv6Address::ptr(new IPv6Address(subnet));
}

uint32_t IPv6Address::getPort() const {
    return byteswapOnLittleEndian(m_addr.sin6_port);
}

void IPv6Address::setPort(uint16_t v) {
    m_addr.sin6_port = byteswapOnLittleEndian(v);
}

//MAX_PATH_LEN 表示在 Unix 域套接字地址中，路径字符串的最大长度，不包括终止符 '\0'。
static const size_t MAX_PATH_LEN = sizeof(((sockaddr_un*)0)->sun_path) - 1;

//创建一个用于 Unix 域套接字通信的地址对象，并对其进行初始化
UnixAddress::UnixAddress() {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sun_family = AF_UNIX;
    m_length = offsetof(sockaddr_un, sun_path) + MAX_PATH_LEN;
}

//根据传入的路径字符串创建一个 Unix 域地址对象，并进行必要的初始化，以便在后续的通信操作中使用。
UnixAddress::UnixAddress(const std::string& path) {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sun_family = AF_UNIX;
    m_length = path.size() + 1; //路径字符串长度加上 1（用于存储终止符 '\0'）

    //如果路径不为空且以终止符 '\0' 开头（表示抽象路径），则将 m_length 减去 1，因为抽象路径在 sun_path 中不需要额外的终止符。
    if(!path.empty() && path[0] == '\0') {
        --m_length;
    }

    if(m_length > sizeof(m_addr.sun_path)) {
        throw std::logic_error("path too long");
    }
    memcpy(m_addr.sun_path, path.c_str(), m_length);
    m_length += offsetof(sockaddr_un, sun_path);
}

void UnixAddress::setAddrLen(uint32_t v) {
    m_length = v;
}

sockaddr* UnixAddress::getAddr() {
    return (sockaddr*)&m_addr;
}

const sockaddr* UnixAddress::getAddr() const {
    return (sockaddr*)&m_addr;
}

socklen_t UnixAddress::getAddrLen() const {
    return m_length;
}

//insert 函数会将 UnixAddress 对象的地址信息以一种合适的形式插入到输出流中，便于进行打印输出。
std::ostream& UnixAddress::insert(std::ostream& os) const {
    if(m_length > offsetof(sockaddr_un, sun_path)
            && m_addr.sun_path[0] == '\0') {
        return os << "\\0" << std::string(m_addr.sun_path + 1,
                m_length - offsetof(sockaddr_un, sun_path) - 1);
    }
    return os << m_addr.sun_path;
}

//创建未知类型的地址对象
UnknownAddress::UnknownAddress(int family) {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sa_family = family;
}

UnknownAddress::UnknownAddress(const sockaddr& addr) {
    m_addr = addr;
}

sockaddr* UnknownAddress::getAddr() {
    return (sockaddr*)&m_addr;
}

const sockaddr* UnknownAddress::getAddr() const {
    return &m_addr;
}

socklen_t UnknownAddress::getAddrLen() const {
    return sizeof(m_addr);
}

//将 UnknownAddress 对象的信息以一种可读性良好的格式插入到输出流中。
//例如，如果 m_addr.sa_family 的值是 10，那么输出可能是 [UnknownAddress family=10]。
std::ostream& UnknownAddress::insert(std::ostream& os) const {
    os << "[UnknownAddress family=" << m_addr.sa_family << "]";
    return os;
}

std::ostream& operator<<(std::ostream& os, const Address& addr) {
    return addr.insert(os);
}

}
