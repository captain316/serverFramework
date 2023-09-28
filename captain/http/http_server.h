/**
 * @file http_server.h
 * @brief HTTP服务器封装
 */

#ifndef __CAPTAIN_HTTP_HTTP_SERVER_H__
#define __CAPTAIN_HTTP_HTTP_SERVER_H__

#include "captain/include/tcp_server.h"
#include "http_session.h"
#include "servlet.h"

namespace captain {
namespace http {

/**
 * @brief HTTP服务器类
 */
class HttpServer : public TcpServer {
public:
    /// 智能指针类型
    typedef std::shared_ptr<HttpServer> ptr;

    /**
     * @brief 构造函数
     * @param[in] keepalive 是否长连接
     * @param[in] worker 工作调度器
     * @param[in] accept_worker 接收连接调度器
     */
    HttpServer(bool keepalive = false
               ,captain::IOManager* worker = captain::IOManager::GetThis()
               ,captain::IOManager* io_worker = captain::IOManager::GetThis()
               ,captain::IOManager* accept_worker = captain::IOManager::GetThis());

    /**
     * @brief 获取ServletDispatch
     */
    ServletDispatch::ptr getServletDispatch() const { return m_dispatch;}

    /**
     * @brief 设置ServletDispatch
     */
    void setServletDispatch(ServletDispatch::ptr v) { m_dispatch = v;}

    //virtual void setName(const std::string& v) override;
protected:
    virtual void handleClient(Socket::ptr client) override;
private:
    /// 是否支持长连接
    bool m_isKeepalive;
    /// Servlet分发器
    ServletDispatch::ptr m_dispatch;
};

}
}

#endif
