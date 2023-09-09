#include "captain/include/address.h"
#include "captain/include/log.h"

captain::Logger::ptr g_logger = CAPTAIN_LOG_ROOT();

void test() {
    std::vector<captain::Address::ptr> addrs;

    CAPTAIN_LOG_INFO(g_logger) << "begin";
    bool v = captain::Address::Lookup(addrs, "www.baidu.com", AF_INET);
    //bool v = captain::Address::Lookup(addrs, "www.captain.top", AF_INET);
    CAPTAIN_LOG_INFO(g_logger) << "end";
    if(!v) {
        CAPTAIN_LOG_ERROR(g_logger) << "lookup fail";
        return;
    }

    for(size_t i = 0; i < addrs.size(); ++i) {
        CAPTAIN_LOG_INFO(g_logger) << i << " - " << addrs[i]->toString();
    }
}

void test_iface() {
    std::multimap<std::string, std::pair<captain::Address::ptr, uint32_t> > results;

    bool v = captain::Address::GetInterfaceAddresses(results);
    if(!v) {
        CAPTAIN_LOG_ERROR(g_logger) << "GetInterfaceAddresses fail";
        return;
    }

    for(auto& i: results) {
        CAPTAIN_LOG_INFO(g_logger) << i.first << " - " << i.second.first->toString() << " - "
            << i.second.second;
    }
}

void test_ipv4() {
    //auto addr = captain::IPAddress::Create("www.sylar.top");
    //auto addr = captain::IPAddress::Create("www.baidu.com");
    auto addr = captain::IPAddress::Create("127.0.0.8");
    if(addr) {
        CAPTAIN_LOG_INFO(g_logger) << addr->toString();
    }
}

int main(int argc, char** argv) {
    //test_ipv4();
    //test_iface();
    test();
    return 0;
}
