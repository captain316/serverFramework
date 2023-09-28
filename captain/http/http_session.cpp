#include "http_session.h"
#include "http_parser.h"

namespace captain {
namespace http {

HttpSession::HttpSession(Socket::ptr sock, bool owner)
    :SocketStream(sock, owner) {
}

HttpRequest::ptr HttpSession::recvRequest() {
    HttpRequestParser::ptr parser(new HttpRequestParser); //解析 HTTP 请求
    //获取用于解析的缓冲区大小，并创建一个相应大小的字符数组 buffer 用于接收数据。
    uint64_t buff_size = HttpRequestParser::GetHttpRequestBufferSize();
    //uint64_t buff_size = 100;
    std::shared_ptr<char> buffer(
            new char[buff_size], [](char* ptr){
                delete[] ptr;
            });
    char* data = buffer.get();
    int offset = 0;
    do {
        // 从客户端读取数据到缓冲区
        int len = read(data + offset, buff_size - offset);
        if(len <= 0) {
            close();
            return nullptr;
        }
        len += offset;
        size_t nparse = parser->execute(data, len);
        // 检查HTTP解析是否出错
        if(parser->hasError()) {
            close();
            return nullptr;
        }

        // 如果缓冲区满了，关闭连接并返回空指针
        offset = len - nparse;
        if(offset == (int)buff_size) {
            close();
            return nullptr;
        }

        // 如果HTTP请求已完成解析，跳出循环
        if(parser->isFinished()) {
            break;
        }
    } while(true);
    int64_t length = parser->getContentLength();
    if(length > 0) {
        std::string body;
        body.resize(length);

        int len = 0;
        if(length >= offset) {
            memcpy(&body[0], data, offset);
            len = offset;
        } else {
            memcpy(&body[0], data, length);
            len = length;
        }
        length -= offset;
        // 如果还有数据需要接收，调用 readFixSize 函数接收剩余的数据
        if(length > 0) {
            if(readFixSize(&body[len], length) <= 0) {
                close();
                return nullptr;
            }
        }
        // 将消息体设置到解析得到的 HttpRequest 对象中
        parser->getData()->setBody(body);
    }
    // 初始化解析得到的 HttpRequest 对象
    //parser->getData()->init();
    // 返回解析得到的 HttpRequest 对象的智能指针
    return parser->getData();
}

int HttpSession::sendResponse(HttpResponse::ptr rsp) {
    std::stringstream ss;
    ss << *rsp;
    std::string data = ss.str();
    return writeFixSize(data.c_str(), data.size());
}

}
}
