#include "captain/include/socket.h"
#include "captain/include/captain.h"
#include "captain/include/iomanager.h"

static captain::Logger::ptr g_looger = CAPTAIN_LOG_ROOT();

void test_socket() {
    //std::vector<captain::Address::ptr> addrs;
    //captain::Address::Lookup(addrs, "www.baidu.com", AF_INET);
    //captain::IPAddress::ptr addr;
    //for(auto& i : addrs) {
    //    CAPTAIN_LOG_INFO(g_looger) << i->toString();
    //    addr = std::dynamic_pointer_cast<captain::IPAddress>(i);
    //    if(addr) {
    //        break;
    //    }
    //}

    captain::IPAddress::ptr addr = captain::Address::LookupAnyIPAddress("www.baidu.com");
    //captain::IPAddress::ptr addr = captain::Address::LookupAnyIPAddress("www.baidu.com",AF_UNSPEC);
    if(addr) {
        CAPTAIN_LOG_INFO(g_looger) << "get address: " << addr->toString();
    } else {
        CAPTAIN_LOG_ERROR(g_looger) << "get address fail";
        return;
    }

    captain::Socket::ptr sock = captain::Socket::CreateTCP(addr);
    addr->setPort(80);
    CAPTAIN_LOG_INFO(g_looger) << "addr=" << addr->toString();
    if(!sock->connect(addr)) {
        CAPTAIN_LOG_ERROR(g_looger) << "connect " << addr->toString() << " fail";
        return;
    } else {
        CAPTAIN_LOG_INFO(g_looger) << "connect " << addr->toString() << " connected";
    }

    const char buff[] = "GET / HTTP/1.0\r\n\r\n";
    int rt = sock->send(buff, sizeof(buff));
    if(rt <= 0) {
        CAPTAIN_LOG_INFO(g_looger) << "send fail rt=" << rt;
        return;
    }

    std::string buffs;
    buffs.resize(4096);
    rt = sock->recv(&buffs[0], buffs.size());

    if(rt <= 0) {
        CAPTAIN_LOG_INFO(g_looger) << "recv fail rt=" << rt;
        return;
    }

    buffs.resize(rt);
    CAPTAIN_LOG_INFO(g_looger) << buffs;
}

int main(int argc, char** argv) {
    captain::IOManager iom;
    iom.schedule(&test_socket);
    return 0;
}
