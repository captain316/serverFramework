#include "captain/include/tcp_server.h"
#include "captain/include/iomanager.h"
#include "captain/include/log.h"

captain::Logger::ptr g_logger = CAPTAIN_LOG_ROOT();

void run() {
    auto addr = captain::Address::LookupAny("0.0.0.0:8033");
    //auto addr2 = captain::UnixAddress::ptr(new captain::UnixAddress("/tmp/unix_addr"));
    //CAPTAIN_LOG_INFO(g_logger) << *addr << " - " << *addr2;
    std::vector<captain::Address::ptr> addrs;
    addrs.push_back(addr);
    //addrs.push_back(addr2);

    captain::TcpServer::ptr tcp_server(new captain::TcpServer);
    std::vector<captain::Address::ptr> fails;
    while(!tcp_server->bind(addrs, fails)) {
        sleep(2);
    }
    tcp_server->start();
    
}
int main(int argc, char** argv) {
    captain::IOManager iom(2);
    iom.schedule(run);
    return 0;
}
