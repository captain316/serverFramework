#include "captain/include/socket.h"
#include "captain/include/log.h"
#include "captain/include/iomanager.h"

static captain::Logger::ptr g_logger = CAPTAIN_LOG_ROOT();

void run() {
    captain::IPAddress::ptr addr = captain::Address::LookupAnyIPAddress("0.0.0.0:8050");
    captain::Socket::ptr sock = captain::Socket::CreateUDP(addr);
    if(sock->bind(addr)) {
        CAPTAIN_LOG_INFO(g_logger) << "udp bind : " << *addr;
    } else {
        CAPTAIN_LOG_ERROR(g_logger) << "udp bind : " << *addr << " fail";
        return;
    }
    while(true) {
        char buff[1024];
        captain::Address::ptr from(new captain::IPv4Address);
        int len = sock->recvFrom(buff, 1024, from);
        if(len > 0) {
            buff[len] = '\0';
            CAPTAIN_LOG_INFO(g_logger) << "recv: " << buff << " from: " << *from;
            len = sock->sendTo(buff, len, from);
            if(len < 0) {
                CAPTAIN_LOG_INFO(g_logger) << "send: " << buff << " to: " << *from
                    << " error=" << len;
            }
        }
    }
}

int main(int argc, char** argv) {
    captain::IOManager iom(1);
    iom.schedule(run);
    return 0;
}
