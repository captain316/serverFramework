#include "captain/http/http_server.h"
#include "captain/include/log.h"

static captain::Logger::ptr g_logger = CAPTAIN_LOG_ROOT();

#define XX(...) #__VA_ARGS__


captain::IOManager::ptr worker;
void run() {
    g_logger->setLevel(captain::LogLevel::INFO);
    //captain::http::HttpServer::ptr server(new captain::http::HttpServer(true, worker.get(), captain::IOManager::GetThis()));
    captain::http::HttpServer::ptr server(new captain::http::HttpServer(true));
    captain::Address::ptr addr = captain::Address::LookupAnyIPAddress("0.0.0.0:8020");
    while(!server->bind(addr)) {
        sleep(2);
    }
    auto sd = server->getServletDispatch();
    sd->addServlet("/captain/xx", [](captain::http::HttpRequest::ptr req
                ,captain::http::HttpResponse::ptr rsp
                ,captain::http::HttpSession::ptr session) {
            rsp->setBody(req->toString());
            return 0;
    });

    sd->addGlobServlet("/captain/*", [](captain::http::HttpRequest::ptr req
                ,captain::http::HttpResponse::ptr rsp
                ,captain::http::HttpSession::ptr session) {
            rsp->setBody("Glob:\r\n" + req->toString());
            return 0;
    });

//     sd->addGlobServlet("/captainx/*", [](captain::http::HttpRequest::ptr req
//                 ,captain::http::HttpResponse::ptr rsp
//                 ,captain::http::HttpSession::ptr session) {
//             rsp->setBody(XX(<html>
// <head><title>404 Not Found</title></head>
// <body>
// <center><h1>404 Not Found</h1></center>
// <hr><center>nginx/1.16.0</center>
// </body>
// </html>
// <!-- a padding to disable MSIE and Chrome friendly error page -->
// <!-- a padding to disable MSIE and Chrome friendly error page -->
// <!-- a padding to disable MSIE and Chrome friendly error page -->
// <!-- a padding to disable MSIE and Chrome friendly error page -->
// <!-- a padding to disable MSIE and Chrome friendly error page -->
// <!-- a padding to disable MSIE and Chrome friendly error page -->
// ));
//             return 0;
//     });


    server->start();
}

int main(int argc, char** argv) {
    captain::IOManager iom(2);
    // captain::IOManager iom(1, true, "main");
    // worker.reset(new captain::IOManager(3, false, "worker"));
    iom.schedule(run);
    return 0;
}
