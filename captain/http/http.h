#ifndef __CAPTAIN_HTTP_HTTP_H__
#define __CAPTAIN_HTTP_HTTP_H__

#include <memory>
#include <string>
#include <map>
#include <iostream>
#include <sstream>
#include <boost/lexical_cast.hpp>

namespace captain {
//子namespace
namespace http {

/* 
定义了两个宏，用于生成 HTTP 请求方法和状态码的映射。

HTTP_METHOD_MAP(XX) 宏生成了一个枚举，其中包含了 HTTP 请求方法的名称和对应的整数值。
例如，GET 方法对应整数值 1，POST 方法对应整数值 3。这个宏的目的是将请求方法的名称映射到整数值，以便在代码中更方便地使用。

HTTP_STATUS_MAP(XX) 宏生成了一个枚举，其中包含了 HTTP 响应状态码的名称和对应的整数值。
例如，OK 状态码对应整数值 200，NOT_FOUND 状态码对应整数值 404。这个宏的目的是将响应状态码的名称映射到整数值，以便在代码中更方便地使用。
*/

/* Request Methods */
#define HTTP_METHOD_MAP(XX)         \
  XX(0,  DELETE,      DELETE)       \
  XX(1,  GET,         GET)          \
  XX(2,  HEAD,        HEAD)         \
  XX(3,  POST,        POST)         \
  XX(4,  PUT,         PUT)          \
  /* pathological */                \
  XX(5,  CONNECT,     CONNECT)      \
  XX(6,  OPTIONS,     OPTIONS)      \
  XX(7,  TRACE,       TRACE)        \
  /* WebDAV */                      \
  XX(8,  COPY,        COPY)         \
  XX(9,  LOCK,        LOCK)         \
  XX(10, MKCOL,       MKCOL)        \
  XX(11, MOVE,        MOVE)         \
  XX(12, PROPFIND,    PROPFIND)     \
  XX(13, PROPPATCH,   PROPPATCH)    \
  XX(14, SEARCH,      SEARCH)       \
  XX(15, UNLOCK,      UNLOCK)       \
  XX(16, BIND,        BIND)         \
  XX(17, REBIND,      REBIND)       \
  XX(18, UNBIND,      UNBIND)       \
  XX(19, ACL,         ACL)          \
  /* subversion */                  \
  XX(20, REPORT,      REPORT)       \
  XX(21, MKACTIVITY,  MKACTIVITY)   \
  XX(22, CHECKOUT,    CHECKOUT)     \
  XX(23, MERGE,       MERGE)        \
  /* upnp */                        \
  XX(24, MSEARCH,     M-SEARCH)     \
  XX(25, NOTIFY,      NOTIFY)       \
  XX(26, SUBSCRIBE,   SUBSCRIBE)    \
  XX(27, UNSUBSCRIBE, UNSUBSCRIBE)  \
  /* RFC-5789 */                    \
  XX(28, PATCH,       PATCH)        \
  XX(29, PURGE,       PURGE)        \
  /* CalDAV */                      \
  XX(30, MKCALENDAR,  MKCALENDAR)   \
  /* RFC-2068, section 19.6.1.2 */  \
  XX(31, LINK,        LINK)         \
  XX(32, UNLINK,      UNLINK)       \
  /* icecast */                     \
  XX(33, SOURCE,      SOURCE)       \

/* Status Codes */
#define HTTP_STATUS_MAP(XX)                                                 \
  XX(100, CONTINUE,                        Continue)                        \
  XX(101, SWITCHING_PROTOCOLS,             Switching Protocols)             \
  XX(102, PROCESSING,                      Processing)                      \
  XX(200, OK,                              OK)                              \
  XX(201, CREATED,                         Created)                         \
  XX(202, ACCEPTED,                        Accepted)                        \
  XX(203, NON_AUTHORITATIVE_INFORMATION,   Non-Authoritative Information)   \
  XX(204, NO_CONTENT,                      No Content)                      \
  XX(205, RESET_CONTENT,                   Reset Content)                   \
  XX(206, PARTIAL_CONTENT,                 Partial Content)                 \
  XX(207, MULTI_STATUS,                    Multi-Status)                    \
  XX(208, ALREADY_REPORTED,                Already Reported)                \
  XX(226, IM_USED,                         IM Used)                         \
  XX(300, MULTIPLE_CHOICES,                Multiple Choices)                \
  XX(301, MOVED_PERMANENTLY,               Moved Permanently)               \
  XX(302, FOUND,                           Found)                           \
  XX(303, SEE_OTHER,                       See Other)                       \
  XX(304, NOT_MODIFIED,                    Not Modified)                    \
  XX(305, USE_PROXY,                       Use Proxy)                       \
  XX(307, TEMPORARY_REDIRECT,              Temporary Redirect)              \
  XX(308, PERMANENT_REDIRECT,              Permanent Redirect)              \
  XX(400, BAD_REQUEST,                     Bad Request)                     \
  XX(401, UNAUTHORIZED,                    Unauthorized)                    \
  XX(402, PAYMENT_REQUIRED,                Payment Required)                \
  XX(403, FORBIDDEN,                       Forbidden)                       \
  XX(404, NOT_FOUND,                       Not Found)                       \
  XX(405, METHOD_NOT_ALLOWED,              Method Not Allowed)              \
  XX(406, NOT_ACCEPTABLE,                  Not Acceptable)                  \
  XX(407, PROXY_AUTHENTICATION_REQUIRED,   Proxy Authentication Required)   \
  XX(408, REQUEST_TIMEOUT,                 Request Timeout)                 \
  XX(409, CONFLICT,                        Conflict)                        \
  XX(410, GONE,                            Gone)                            \
  XX(411, LENGTH_REQUIRED,                 Length Required)                 \
  XX(412, PRECONDITION_FAILED,             Precondition Failed)             \
  XX(413, PAYLOAD_TOO_LARGE,               Payload Too Large)               \
  XX(414, URI_TOO_LONG,                    URI Too Long)                    \
  XX(415, UNSUPPORTED_MEDIA_TYPE,          Unsupported Media Type)          \
  XX(416, RANGE_NOT_SATISFIABLE,           Range Not Satisfiable)           \
  XX(417, EXPECTATION_FAILED,              Expectation Failed)              \
  XX(421, MISDIRECTED_REQUEST,             Misdirected Request)             \
  XX(422, UNPROCESSABLE_ENTITY,            Unprocessable Entity)            \
  XX(423, LOCKED,                          Locked)                          \
  XX(424, FAILED_DEPENDENCY,               Failed Dependency)               \
  XX(426, UPGRADE_REQUIRED,                Upgrade Required)                \
  XX(428, PRECONDITION_REQUIRED,           Precondition Required)           \
  XX(429, TOO_MANY_REQUESTS,               Too Many Requests)               \
  XX(431, REQUEST_HEADER_FIELDS_TOO_LARGE, Request Header Fields Too Large) \
  XX(451, UNAVAILABLE_FOR_LEGAL_REASONS,   Unavailable For Legal Reasons)   \
  XX(500, INTERNAL_SERVER_ERROR,           Internal Server Error)           \
  XX(501, NOT_IMPLEMENTED,                 Not Implemented)                 \
  XX(502, BAD_GATEWAY,                     Bad Gateway)                     \
  XX(503, SERVICE_UNAVAILABLE,             Service Unavailable)             \
  XX(504, GATEWAY_TIMEOUT,                 Gateway Timeout)                 \
  XX(505, HTTP_VERSION_NOT_SUPPORTED,      HTTP Version Not Supported)      \
  XX(506, VARIANT_ALSO_NEGOTIATES,         Variant Also Negotiates)         \
  XX(507, INSUFFICIENT_STORAGE,            Insufficient Storage)            \
  XX(508, LOOP_DETECTED,                   Loop Detected)                   \
  XX(510, NOT_EXTENDED,                    Not Extended)                    \
  XX(511, NETWORK_AUTHENTICATION_REQUIRED, Network Authentication Required) \

//该宏会展开为一系列的枚举值，每个枚举值代表一个 HTTP 请求方法。
enum class HttpMethod {
#define XX(num, name, string) name = num,
    HTTP_METHOD_MAP(XX)
#undef XX
    INVALID_METHOD
};

//展开为一系列的枚举值，每个枚举值代表一个 HTTP 响应状态码。
enum class HttpStatus {
#define XX(code, name, desc) name = code,
    HTTP_STATUS_MAP(XX)
#undef XX
};

//这些函数用于将 HTTP 请求方法（HttpMethod）和 HTTP 响应状态码（HttpStatus）与字符串之间进行相互转换。
HttpMethod StringToHttpMethod(const std::string& m);
HttpMethod CharsToHttpMethod(const char* m);
const char* HttpMethodToString(const HttpMethod& m);
const char* HttpStatusToString(const HttpStatus& s);

//使用 CaseInsensitiveLess 结构体作为比较器，可以在不考虑字母大小写的情况下对字符串进行排序或比较。
//这在处理 HTTP 请求和响应的头部信息时非常有用，因为 HTTP 头部字段名称通常不区分大小写。
struct CaseInsensitiveLess {
    bool operator()(const std::string& lhs, const std::string& rhs) const;
};

/**
 * @brief 从一个关联容器类型 MapType 中查找指定键 key，并尝试将对应的值解析为类型 T。
 * 如果键 key 存在并且解析成功，则将解析后的值存储到参数 val 中，并返回 true；否则，
 * 使用默认值 def 初始化 val，并返回 false。
 * 
 * @tparam MapType 关联容器类型，通常是 std::map 或类似容器
 * @tparam T 要解析的目标类型
 * @param m 关联容器，用于查找键值对
 * @param key 要查找的键
 * @param val 用于存储解析后的值的参数
 * @param def 当键 key 不存在或解析失败时使用的默认值
 * @return 如果找到键 key 并成功解析其对应的值则返回 true，否则返回 false
 */
template<class MapType, class T>
bool checkGetAs(const MapType& m, const std::string& key, T& val, const T& def = T()) {
    auto it = m.find(key);
    if(it == m.end()) {
        val = def;
        return false;
    }
    try {
        val = boost::lexical_cast<T>(it->second);
        return true;
    } catch (...) {
        val = def;
    }
    return false;
}


/**
 * @brief 从一个关联容器类型 MapType 中查找指定键 key，并尝试将对应的值解析为类型 T。
 * 如果键 key 存在并且解析成功，则返回解析后的值；否则，返回默认值 def。
 */
template<class MapType, class T>
T getAs(const MapType& m, const std::string& key, const T& def = T()) {
    auto it = m.find(key);
    if(it == m.end()) {
        return def;
    }
    try {
        return boost::lexical_cast<T>(it->second);
    } catch (...) {
    }
    return def;
}


//HTTP请求
class HttpRequest {
public:
    typedef std::shared_ptr<HttpRequest> ptr;
    //在这个 std::map 中，键的比较会忽略大小写，因此，例如，"Header" 和 "header" 被认为是相同的键。
    typedef std::map<std::string, std::string, CaseInsensitiveLess> MapType; 
    //这是 HttpRequest 类的构造函数的声明。version 参数的默认值是 0x11，表示 HTTP 版本号，默认为 1.1。close 参数的默认值是 true，表示在请求处理完成后是否关闭连接。
    HttpRequest(uint8_t version = 0x11, bool close = true);

    HttpMethod getMethod() const { return m_method;}
    uint8_t getVersion() const { return m_version;}
    const std::string& getPath() const { return m_path;}
    const std::string& getQuery() const { return m_query;}
    const std::string& getBody() const { return m_body;}
    const MapType& getHeaders() const { return m_headers;}
    const MapType& getParams() const { return m_params;}
    const MapType& getCookies() const { return m_cookies;}

    void setMethod(HttpMethod v) { m_method = v;}
    void setVersion(uint8_t v) { m_version = v;}
    void setPath(const std::string& v) { m_path = v;}
    void setQuery(const std::string& v) { m_query = v;}
    void setFragment(const std::string& v) { m_fragment = v;}
    void setBody(const std::string& v) { m_body = v;}

    bool isClose() const { return m_close;}
    void setClose(bool v) { m_close = v;}

    void setHeaders(const MapType& v) { m_headers = v;}
    void setParams(const MapType& v) { m_params = v;}
    void setCookies(const MapType& v) { m_cookies = v;}

    std::string getHeader(const std::string& key, const std::string& def = "") const;
    std::string getParam(const std::string& key, const std::string& def = "") const;
    std::string getCookie(const std::string& key, const std::string& def = "") const;

    void setHeader(const std::string& key, const std::string& val);
    void setParam(const std::string& key, const std::string& val);
    void setCookie(const std::string& key, const std::string& val);

    void delHeader(const std::string& key);
    void delParam(const std::string& key);
    void delCookie(const std::string& key);

    bool hasHeader(const std::string& key, std::string* val = nullptr);
    bool hasParam(const std::string& key, std::string* val = nullptr);
    bool hasCookie(const std::string& key, std::string* val = nullptr);

    template<class T>
    bool checkGetHeaderAs(const std::string& key, T& val, const T& def = T()) {
        return checkGetAs(m_headers, key, val, def);
    }

    template<class T>
    T getHeaderAs(const std::string& key, const T& def = T()) {
        return getAs(m_headers, key, def);
    }

    template<class T>
    bool checkGetParamAs(const std::string& key, T& val, const T& def = T()) {
        return checkGetAs(m_headers, key, val, def);
    }

    template<class T>
    T getParamAs(const std::string& key, const T& def = T()) {
        return getAs(m_headers, key, def);
    }

    template<class T>
    bool checkGetCookieAs(const std::string& key, T& val, const T& def = T()) {
        return checkGetAs(m_headers, key, val, def);
    }

    template<class T>
    T getCookieAs(const std::string& key, const T& def = T()) {
        return getAs(m_headers, key, def);
    }

    std::ostream& dump(std::ostream& os) const;
    std::string toString() const;
private:
private:
    HttpMethod m_method; //存储 HTTP 请求方法的枚举值，使用 HttpMethod 类型
    uint8_t m_version; //存储 HTTP 请求的版本信息
    bool m_close;

    std::string m_path; //存储请求的路径部分，通常是 URL 中的路径部分，例如 "/example/resource"。
    std::string m_query; //存储请求的查询参数，通常是 URL 中的查询字符串，例如 "?param1=value1&param2=value2"。
    std::string m_fragment; //存储 URL 片段标识符（Fragment Identifier），通常以 "#" 开头，用于标识文档中的某个特定部分。
    std::string m_body; //存储 HTTP 请求的消息体（Message Body），通常包含请求的具体内容，例如 POST 请求的表单数据。

    MapType m_headers; //存储 HTTP 请求的头部信息。这是一个键值对映射，其中键表示头部字段名，值表示头部字段的值。
    MapType m_params; //存储 HTTP 请求的参数。与头部信息类似，这也是一个键值对映射，用于存储 URL 查询参数。
    MapType m_cookies; //存储 HTTP 请求中的 Cookie 信息。同样，这也是一个键值对映射，用于存储各个 Cookie 的键值对。
};

class HttpResponse {
public:
    typedef std::shared_ptr<HttpResponse> ptr;
    typedef std::map<std::string, std::string, CaseInsensitiveLess> MapType;
    // 构造函数，可指定版本号和连接状态，默认为 HTTP/1.1 和关闭连接
    HttpResponse(uint8_t version = 0x11, bool close = true);
    // 获取响应状态码
    HttpStatus getStatus() const { return m_status;}
    // 获取HTTP版本号
    uint8_t getVersion() const { return m_version;}
    // 获取响应体
    const std::string& getBody() const { return m_body;}
    // 获取响应原因短语
    const std::string& getReason() const { return m_reason;}
    // 获取响应头部字段
    const MapType& getHeaders() const { return m_headers;}

    // 设置响应状态码
    void setStatus(HttpStatus v) { m_status = v;}
    // 设置HTTP版本号
    void setVersion(uint8_t v) { m_version = v;}
    // 设置响应体
    void setBody(const std::string& v) { m_body = v;}
    // 设置响应原因短语
    void setReason(const std::string& v) { m_reason = v;}
    // 设置响应头部字段
    void setHeaders(const MapType& v) { m_headers = v;}

    // 检查是否要关闭连接
    bool isClose() const { return m_close;}
    // 设置是否要关闭连接
    void setClose(bool v) { m_close = v;}

    // 获取指定名称的响应头部字段的值，如果不存在则返回默认值
    std::string getHeader(const std::string& key, const std::string& def = "") const;
    // 设置指定名称的响应头部字段的值
    void setHeader(const std::string& key, const std::string& val);
    // 删除指定名称的响应头部字段
    void delHeader(const std::string& key);

    // 用于检查指定名称的响应头部字段，并将其值转换为指定类型 T
    template<class T>
    bool checkGetHeaderAs(const std::string& key, T& val, const T& def = T()) {
        return checkGetAs(m_headers, key, val, def);
    }

    // 获取指定名称的响应头部字段的值，并将其转换为指定类型 T，如果字段不存在则返回默认值
    template<class T>
    T getHeaderAs(const std::string& key, const T& def = T()) {
        return getAs(m_headers, key, def);
    }

    // 将 HttpResponse 对象的内容格式化为字符串
    std::ostream& dump(std::ostream& os) const;
    // 获取 HttpResponse 对象的内容并返回作为字符串
    std::string toString() const;
private:
    HttpStatus m_status;    // 响应状态码
    uint8_t m_version;      // HTTP版本号
    bool m_close;           // 连接状态
    std::string m_body;     // 响应体
    std::string m_reason;   // 响应原因短语
    MapType m_headers;      // 响应头部字段
};

/**
 * @brief 流式输出HttpRequest
 * @param[in, out] os 输出流
 * @param[in] req HTTP请求
 * @return 输出流
 */
std::ostream& operator<<(std::ostream& os, const HttpRequest& req);

/**
 * @brief 流式输出HttpResponse
 * @param[in, out] os 输出流
 * @param[in] rsp HTTP响应
 * @return 输出流
 */
std::ostream& operator<<(std::ostream& os, const HttpResponse& rsp);

}
}

#endif
