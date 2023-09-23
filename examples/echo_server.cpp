#include "captain/include/tcp_server.h"
#include "captain/include/log.h"
#include "captain/include/iomanager.h"
#include "captain/include/bytearray.h"
#include "captain/include/address.h"

/* 实现了一个简单的 Echo 服务器，可以选择不同的模式来输出客户端发送的数据。
**如果以 -t 参数运行程序，将以文本形式输出数据；如果以 -b 参数运行程序，将以十六进制形式输出数据。
**用户可以使用 Telnet 工具连接到服务器，然后输入数据，服务器将回显用户输入的数据。
**./bin/echo_server -t
**telnet 127.0.0.1 8020    hello captain
**
*/

static captain::Logger::ptr g_logger = CAPTAIN_LOG_ROOT();

class EchoServer : public captain::TcpServer {
public:
    EchoServer(int type); //指定服务器类型，以便后续的数据回显。
    void handleClient(captain::Socket::ptr client); //处理客户端连接。

private:
    int m_type = 0;
};

EchoServer::EchoServer(int type)
    :m_type(type) {
}

void EchoServer::handleClient(captain::Socket::ptr client) {
    CAPTAIN_LOG_INFO(g_logger) << "handleClient " << *client;   
    //创建一个 captain::ByteArray 对象 ba，用于存储接收到的数据。
    captain::ByteArray::ptr ba(new captain::ByteArray);
    //进入一个无限循环，不断接收客户端发送的数据。
    while(true) {
        //清空字节数组，以便存储新的数据。
        ba->clear();
        std::vector<iovec> iovs;
        //获取可写入数据的缓冲区。
        ba->getWriteBuffers(iovs, 1024);

        //使用 client->recv 接收数据，并将数据存储在缓冲区中。
        int rt = client->recv(&iovs[0], iovs.size());
        //检查接收的数据，如果返回值 rt 为0，表示客户端关闭连接，退出循环。如果 rt 小于0，表示出现错误，记录错误信息并退出循环。
        if(rt == 0) {
            CAPTAIN_LOG_INFO(g_logger) << "client close: " << *client;
            break;
        } else if(rt < 0) {
            CAPTAIN_LOG_INFO(g_logger) << "client error rt=" << rt
                << " errno=" << errno << " errstr=" << strerror(errno);
            break;
        }
        //设置字节数组的位置，并将其重置为0。
        ba->setPosition(ba->getPosition() + rt);
        ba->setPosition(0);
        //CAPTAIN_LOG_INFO(g_logger) << "recv rt=" << rt << " data=" << std::string((char*)iovs[0].iov_base, rt);
        
        //根据服务器类型 m_type，将接收到的数据输出到标准输出。如果是文本模式，将数据以文本形式输出；如果是二进制模式，将数据以十六进制形式输出。
        if(m_type == 1) {//text文本 
            std::cout << ba->toString();// << std::endl;
        } else { //二进制
            std::cout << ba->toHexString();// << std::endl;
        }
        std::cout.flush();
    }
}

//指定服务器的类型，默认为文本模式。
int type = 1;

//run 服务器的主要运行逻辑。
void run() {
    CAPTAIN_LOG_INFO(g_logger) << "server type=" << type; //记录服务器类型信息。
    //创建 EchoServer 对象 es，并根据 type 指定服务器类型。
    EchoServer::ptr es(new EchoServer(type));
    // 查找可用的本地地址，并将其存储在 addr 中。
    auto addr = captain::Address::LookupAny("0.0.0.0:8020");
    //使用 es->bind 方法绑定地址，如果绑定失败，则每隔2秒重试，直到成功为止。
    while(!es->bind(addr)) {
        sleep(2);
    }
    //启动服务器，开始监听客户端连接。
    es->start();
}

int main(int argc, char** argv) {
    //检查命令行参数，如果参数不足，则输出用法信息并退出。
    //根据命令行参数设置 type 的值，可以选择使用 -t 参数指定文本模式，或使用 -b 参数指定二进制模式。
    if(argc < 2) {
        CAPTAIN_LOG_INFO(g_logger) << "used as[" << argv[0] << " -t] or [" << argv[0] << " -b]";
        return 0;
    }

    if(!strcmp(argv[1], "-b")) {
        type = 2;
    }

    //创建一个 captain::IOManager 对象 iom，并指定协程的数量为2。
    captain::IOManager iom(2);
    //将 run 函数调度到协程中执行。
    iom.schedule(run);
    return 0;
}
