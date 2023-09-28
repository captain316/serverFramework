#include "http.h"

namespace captain {
namespace http {

//不区分大小写的情况下比较两个字符串的方式。
HttpMethod StringToHttpMethod(const std::string& m) {
#define XX(num, name, string) \
    if(strcmp(#string, m.c_str()) == 0) { \
        return HttpMethod::name; \
    }
    HTTP_METHOD_MAP(XX);
#undef XX
    return HttpMethod::INVALID_METHOD;
}

/* 将一个字符串（字符数组）转换为 HttpMethod 枚举值。
**每个 XX 宏表示了一个 HTTP 请求方法的名称和对应的枚举值。这个函数通过遍历这些宏，尝试将输入的字符数组与宏中的字符串进行比较，
**如果匹配成功，则返回对应的枚举值。
**如果没有匹配成功，则返回 HttpMethod::INVALID_METHOD，表示无效的 HTTP 请求方法。
 */
HttpMethod CharsToHttpMethod(const char* m) {
#define XX(num, name, string) \
    if(strncmp(#string, m, strlen(#string)) == 0) { \
        return HttpMethod::name; \
    }
    HTTP_METHOD_MAP(XX);
#undef XX
    return HttpMethod::INVALID_METHOD;
}

// 定义一个静态数组，用于将 HttpMethod 枚举值映射到字符串表示。
static const char* s_method_string[] = {
#define XX(num, name, string) #string,
    HTTP_METHOD_MAP(XX)
#undef XX
};

// 将 HttpMethod 转换为字符串表示的函数。
const char* HttpMethodToString(const HttpMethod& m) {
    // 将 HttpMethod 枚举值强制类型转换为整数，以获取其索引。
    uint32_t idx = (uint32_t)m;
    // 检查索引是否超出 s_method_string 数组的有效索引范围。
    if(idx >= (sizeof(s_method_string) / sizeof(s_method_string[0]))) {
        // 如果超出范围，则返回 "<unknown>" 表示未知的 HTTP 方法。
        return "<unknown>";
    }
    // 返回 s_method_string 数组中对应索引位置的字符串表示，即 HTTP 方法名称。
    return s_method_string[idx];
}

// 将 HttpStatus 转换为字符串表示的函数。
const char* HttpStatusToString(const HttpStatus& s) {
    // 使用 switch 语句匹配 HttpStatus 枚举值。
    switch(s) {
        // 在这里，代码为每个可能的 HttpStatus 枚举值指定了一个 case 分支。
        // 例如，对于 HttpStatus::OK，它返回字符串 "OK"。
        // 这里的 XX 宏在 HTTP_STATUS_MAP 宏中被定义，用于将状态码与字符串消息关联起来。
#define XX(code, name, msg) \
        case HttpStatus::name: \
            return #msg;
        HTTP_STATUS_MAP(XX);
#undef XX
        default:
            return "<unknown>";
    }
}

//不区分大小写的情况下比较两个字符串
bool CaseInsensitiveLess::operator()(const std::string& lhs
                            ,const std::string& rhs) const {
    return strcasecmp(lhs.c_str(), rhs.c_str()) < 0;
}

//构造函数，用于创建一个 HTTP 请求对象。
HttpRequest::HttpRequest(uint8_t version, bool close)
    :m_method(HttpMethod::GET)
    ,m_version(version)
    ,m_close(close)
    ,m_path("/") {
}

//获取 HTTP 请求头中指定键的值。如果找到了键，返回其对应的值；否则，返回默认值（def）。
std::string HttpRequest::getHeader(const std::string& key
                            ,const std::string& def) const {
    auto it = m_headers.find(key);
    return it == m_headers.end() ? def : it->second;
}

//获取 HTTP 请求中的查询参数的值。如果找到了键，返回其对应的值；否则，返回默认值。
std::string HttpRequest::getParam(const std::string& key
                            ,const std::string& def) const {
    auto it = m_params.find(key);
    return it == m_params.end() ? def : it->second;
}

//获取 HTTP 请求中的 Cookie 的值。如果找到了键，返回其对应的值；否则，返回默认值。
std::string HttpRequest::getCookie(const std::string& key
                            ,const std::string& def) const {
    auto it = m_cookies.find(key);
    return it == m_cookies.end() ? def : it->second;
}

//设置 HTTP 请求头中的键值对。
void HttpRequest::setHeader(const std::string& key, const std::string& val) {
    m_headers[key] = val;
}

//设置 HTTP 请求中的查询参数的键值对。
void HttpRequest::setParam(const std::string& key, const std::string& val) {
    m_params[key] = val;
}

//设置 HTTP 请求中的 Cookie 的键值对。
void HttpRequest::setCookie(const std::string& key, const std::string& val) {
    m_cookies[key] = val;
}

//从 HTTP 请求头中删除指定键的键值对。
void HttpRequest::delHeader(const std::string& key) {
    m_headers.erase(key);
}

//从 HTTP 请求中的查询参数中删除指定键的键值对。
void HttpRequest::delParam(const std::string& key) {
    m_params.erase(key);
}

//从 HTTP 请求中的 Cookie 中删除指定键的键值对。
void HttpRequest::delCookie(const std::string& key) {
    m_cookies.erase(key);
}

//检查请求头中是否包含特定的键，并且如果存在，可以选择获取对应的值。
bool HttpRequest::hasHeader(const std::string& key, std::string* val) {
    auto it = m_headers.find(key);
    if(it == m_headers.end()) {
        return false;
    }
    if(val) {
        *val = it->second;
    }
    return true;
}

//检查请求参数中是否包含指定的键。
bool HttpRequest::hasParam(const std::string& key, std::string* val) {
    auto it = m_params.find(key);
    if(it == m_params.end()) {
        return false;
    }
    if(val) {
        *val = it->second;
    }
    return true;
}

//检查请求中的 Cookie 是否包含指定的键。
bool HttpRequest::hasCookie(const std::string& key, std::string* val) {
    auto it = m_cookies.find(key);
    if(it == m_cookies.end()) {
        return false;
    }
    if(val) {
        *val = it->second;
    }
    return true;
}

//将 HttpRequest 对象的内容格式化为一个字符串
std::string HttpRequest::toString() const {
    std::stringstream ss;
    dump(ss);
    return ss.str();
}

//将 HttpRequest 对象的内容写入到给定的输出流 os 中。(序列化)
/*它首先将请求行、连接状态和请求头部字段写入输出流，然后检查是否有请求体，如果有的话，也将内容长度和请求体数据写入输出流。
函数返回输出流，允许进一步的流式输出或存储到文件或网络套接字中。*/
std::ostream& HttpRequest::dump(std::ostream& os) const {
    //GET /uri HTTP/1.1
    //Host: wwww.captain.top
    //
    //
    os << HttpMethodToString(m_method) << " "
       << m_path
       << (m_query.empty() ? "" : "?")
       << m_query
       << (m_fragment.empty() ? "" : "#")
       << m_fragment
       << " HTTP/"
       << ((uint32_t)(m_version >> 4))
       << "."
       << ((uint32_t)(m_version & 0x0F))
       << "\r\n";
    os << "connection: " << (m_close ? "close" : "keep-alive") << "\r\n";
    for(auto& i : m_headers) {  // 遍历请求头部字段
        if(strcasecmp(i.first.c_str(), "connection") == 0) {
            continue;
        }
        os << i.first << ":" << i.second << "\r\n"; // 写入头部字段和对应的值
    }
    // 如果请求体非空
    if(!m_body.empty()) {
        os << "content-length: " << m_body.size() << "\r\n\r\n"
           << m_body;  // 写入请求体数据
    } else {
        os << "\r\n";
    }
    return os;
}

HttpResponse::HttpResponse(uint8_t version, bool close)
    :m_status(HttpStatus::OK)   // 默认响应状态码为 200 OK
    ,m_version(version)         // 设置HTTP版本号
    ,m_close(close) {           // 设置连接状态
}

//获取指定名称的响应头部字段的值。
std::string HttpResponse::getHeader(const std::string& key, const std::string& def) const {
    auto it = m_headers.find(key);
    return it == m_headers.end() ? def : it->second;
}

//设置指定名称的响应头部字段的值。
void HttpResponse::setHeader(const std::string& key, const std::string& val) {
    m_headers[key] = val;
}

//删除指定名称的响应头部字段。
void HttpResponse::delHeader(const std::string& key) {
    m_headers.erase(key);
}

//将整个 HTTP 响应对象转换为字符串形式。
std::string HttpResponse::toString() const {
    std::stringstream ss;
    dump(ss);
    return ss.str();
}

//将 HttpResponse 对象的各个属性按照 HTTP 响应的格式写入到给定的输出流 os 中。它生成了包括响应行、响应头部字段和可选的消息体的完整 HTTP 响应。
std::ostream& HttpResponse::dump(std::ostream& os) const {
    /* 
    这段代码生成了HTTP响应行的字符串，其格式为 HTTP/<主版本号>.<次版本号> <状态码> <原因短语>\r\n，
    例如 HTTP/1.1 200 OK\r\n 表示HTTP/1.1协议的一个成功响应。
     */
    os << "HTTP/"
       << ((uint32_t)(m_version >> 4)) // 解析主版本号（高四位）
       << "."
       << ((uint32_t)(m_version & 0x0F)) // 解析次版本号（低四位）
       << " "
       << (uint32_t)m_status     // 响应状态码
       << " "
       << (m_reason.empty() ? HttpStatusToString(m_status) : m_reason) // 响应原因短语
       << "\r\n"; //这是HTTP协议中行结束符的表示，表示换行。

    // 输出响应头部字段
    for(auto& i : m_headers) {
        // 忽略 "connection" 头部字段
        if(strcasecmp(i.first.c_str(), "connection") == 0) {
            continue;
        }
        os << i.first << ": " << i.second << "\r\n";
    }
    // 输出 "connection" 头部字段，指示是否保持连接
    os << "connection: " << (m_close ? "close" : "keep-alive") << "\r\n";

    if(!m_body.empty()) {
        // 如果响应包含消息体，输出消息体长度
        os << "content-length: " << m_body.size() << "\r\n\r\n"
        << m_body;
    } else {
        // 如果没有消息体，只输出一个空行表示头部字段结束
        os << "\r\n";
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const HttpRequest& req) {
    return req.dump(os);
}

std::ostream& operator<<(std::ostream& os, const HttpResponse& rsp) {
    return rsp.dump(os);
}

}
}
