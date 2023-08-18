#include "captain/include/captain.h"
#include "captain/include/iomanager.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <sys/epoll.h>

captain::Logger::ptr g_logger = CAPTAIN_LOG_ROOT();

int sock = 0;

void test_fiber() {
    CAPTAIN_LOG_INFO(g_logger) << "test_fiber sock=" << sock;

    //sleep(3);

    //close(sock);
    //captain::IOManager::GetThis()->cancelAll(sock);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(sock, F_SETFL, O_NONBLOCK);

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80); 
    //inet_pton(AF_INET, "115.239.210.27", &addr.sin_addr.s_addr);
    inet_pton(AF_INET, "39.156.66.10", &addr.sin_addr.s_addr);

    if(!connect(sock, (const sockaddr*)&addr, sizeof(addr))) {
    } else if(errno == EINPROGRESS) {
        CAPTAIN_LOG_INFO(g_logger) << "add event errno=" << errno << " " << strerror(errno);
        captain::IOManager::GetThis()->addEvent(sock, captain::IOManager::READ, [](){
            CAPTAIN_LOG_INFO(g_logger) << "read callback";
        });
        captain::IOManager::GetThis()->addEvent(sock, captain::IOManager::WRITE, [](){
            CAPTAIN_LOG_INFO(g_logger) << "write callback";
            //close(sock);
            captain::IOManager::GetThis()->cancelEvent(sock, captain::IOManager::READ);
            close(sock);
        });
    } else {
        CAPTAIN_LOG_INFO(g_logger) << "else " << errno << " " << strerror(errno);
    }

}

void test1() {
    std::cout << "EPOLLIN=" << EPOLLIN
              << " EPOLLOUT=" << EPOLLOUT << std::endl;
    captain::IOManager iom(2, false);
    iom.schedule(&test_fiber);
}

captain::Timer::ptr s_timer;
void test_timer() {
    captain::IOManager iom(2);
    s_timer = iom.addTimer(1000, [](){
        static int i = 0;
        CAPTAIN_LOG_INFO(g_logger) << "hello timer i=" << i;
        //CAPTAIN_LOG_INFO(g_logger) << "hello timer";
        if(++i == 3) {
            s_timer->reset(2000, true);
            //s_timer->cancel();
        }
    }, true);
}

int main(int argc, char** argv) {
    //test1();
    test_timer();
    return 0;
}
