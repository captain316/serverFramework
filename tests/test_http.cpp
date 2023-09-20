#include "captain/http/http.h"
#include "captain/include/log.h"

void test_request() {
    captain::http::HttpRequest::ptr req(new captain::http::HttpRequest);
    req->setHeader("host" , "www.sylar.top");
    //req->setHeader("host" , "www.baidu.com");
    req->setBody("hello captain");
    req->dump(std::cout) << std::endl;
}

void test_response() {
    captain::http::HttpResponse::ptr rsp(new captain::http::HttpResponse);
    rsp->setHeader("X-X", "captain");
    rsp->setBody("hello captain");
    rsp->setStatus((captain::http::HttpStatus)400);
    rsp->setClose(false);

    rsp->dump(std::cout) << std::endl;
}

int main(int argc, char** argv) {
    test_request();
    test_response();
    return 0;
}
