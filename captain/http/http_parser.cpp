#include "http_parser.h"
#include "captain/include/log.h"
#include "captain/include/config.h"
#include <string.h>

namespace captain {
namespace http {

static captain::Logger::ptr g_logger = CAPTAIN_LOG_NAME("system");

//配置HTTP请求的缓冲区大小，默认值为 4 * 1024 字节。
static captain::ConfigVar<uint64_t>::ptr g_http_request_buffer_size =
    captain::Config::Lookup("http.request.buffer_size"
                ,(uint64_t)(4 * 1024), "http request buffer size");

//配置HTTP请求的最大消息体大小，默认值为64MB。
static captain::ConfigVar<uint64_t>::ptr g_http_request_max_body_size =
    captain::Config::Lookup("http.request.max_body_size"
                ,(uint64_t)(64 * 1024 * 1024), "http request max body size");

//配置HTTP响应的缓冲区大小，默认值为 4 * 1024 字节。
static captain::ConfigVar<uint64_t>::ptr g_http_response_buffer_size =
    captain::Config::Lookup("http.response.buffer_size"
                ,(uint64_t)(4 * 1024), "http response buffer size");

//配置HTTP响应的最大消息体大小，默认值为64MB。
static captain::ConfigVar<uint64_t>::ptr g_http_response_max_body_size =
    captain::Config::Lookup("http.response.max_body_size"
                ,(uint64_t)(64 * 1024 * 1024), "http response max body size");

//静态的全局变量，这些变量用于存储配置的数值。这些变量的值需要在程序运行前静态地设定，通常在代码中直接赋值。
//这些值在程序运行时保持不变，如果需要修改这些参数，必须重新编译程序并重新运行。
static uint64_t s_http_request_buffer_size = 0;
static uint64_t s_http_request_max_body_size = 0;
static uint64_t s_http_response_buffer_size = 0;
static uint64_t s_http_response_max_body_size = 0;

uint64_t HttpRequestParser::GetHttpRequestBufferSize() {
    return s_http_request_buffer_size;
}

uint64_t HttpRequestParser::GetHttpRequestMaxBodySize() {
    return s_http_request_max_body_size;
}

uint64_t HttpResponseParser::GetHttpResponseBufferSize() {
    return s_http_response_buffer_size;
}

uint64_t HttpResponseParser::GetHttpResponseMaxBodySize() {
    return s_http_response_max_body_size;
}

/* 
在程序启动时，将全局静态变量初始化为与配置变量相等的值，并且为配置变量添加监听器，
以便在配置变量的值发生变化时，及时更新全局静态变量的值。
 */
namespace {
struct _RequestSizeIniter {
    _RequestSizeIniter() {
        //全局静态变量的初始值会和配置变量的初始值相匹配
        s_http_request_buffer_size = g_http_request_buffer_size->getValue();
        s_http_request_max_body_size = g_http_request_max_body_size->getValue();
        s_http_response_buffer_size = g_http_response_buffer_size->getValue();
        s_http_response_max_body_size = g_http_response_max_body_size->getValue();
       
        //当 g_http_request_buffer_size 配置变量的值发生变化时，将最新的值赋给了 s_http_request_buffer_size 这个静态变量
        g_http_request_buffer_size->addListener(
                [](const uint64_t& ov, const uint64_t& nv){
                s_http_request_buffer_size = nv;
        });

        g_http_request_max_body_size->addListener(
                [](const uint64_t& ov, const uint64_t& nv){
                s_http_request_max_body_size = nv;
        });

        g_http_response_buffer_size->addListener(
                [](const uint64_t& ov, const uint64_t& nv){
                s_http_response_buffer_size = nv;
        });

        g_http_response_max_body_size->addListener(
                [](const uint64_t& ov, const uint64_t& nv){
                s_http_response_max_body_size = nv;
        });
    }
};
static _RequestSizeIniter _init;
}
//解析HTTP请求方法的回调函数。
void on_request_method(void *data, const char *at, size_t length) {
    //C++中的类型转换，将一个指向void类型的指针（data）强制转换为一个指向HttpRequestParser类型的指针。这意味着该函数预期 data 参数实际上是一个指向 HttpRequestParser 对象的指针。
    //这种类型转换的前提是你必须确保 data 实际上指向了一个 HttpRequestParser 类的对象，否则会导致未定义行为或程序错误。
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    //将 at 指向的字符数组（HTTP 请求方法字符串）通过 CharsToHttpMethod 函数转换为对应的 HttpMethod 枚举值 m。
    HttpMethod m = CharsToHttpMethod(at);

    if(m == HttpMethod::INVALID_METHOD) {
        CAPTAIN_LOG_WARN(g_logger) << "invalid http request method: "
            << std::string(at, length);
        parser->setError(1000);
        return;
    }
    //将解析得到的 HTTP 请求方法 m 设置到 HTTP 请求对象中
    parser->getData()->setMethod(m);
}

void on_request_uri(void *data, const char *at, size_t length) {
}

void on_request_fragment(void *data, const char *at, size_t length) {
    //CAPTAIN_LOG_INFO(g_logger) << "on_request_fragment:" << std::string(at, length);
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    //将解析得到的片段字符串（由at指向的字符序列，长度为length）设置为HTTP请求对象的片段信息。
    parser->getData()->setFragment(std::string(at, length));
}

void on_request_path(void *data, const char *at, size_t length) {
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    parser->getData()->setPath(std::string(at, length));
}

void on_request_query(void *data, const char *at, size_t length) {
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    parser->getData()->setQuery(std::string(at, length));
}

//解析并设置 HTTP 请求的版本信息
void on_request_version(void *data, const char *at, size_t length) {
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    uint8_t v = 0;
    /* 
    如果 at 和 length 匹配 "HTTP/1.1"，则将版本设置为 0x11，如果匹配 "HTTP/1.0"，
    则将版本设置为 0x10，否则将错误标记为 1001，表示无效的 HTTP 请求版本。
     */
    if(strncmp(at, "HTTP/1.1", length) == 0) {
        v = 0x11;
    } else if(strncmp(at, "HTTP/1.0", length) == 0) {
        v = 0x10;
    } else {
        CAPTAIN_LOG_WARN(g_logger) << "invalid http request version: "
            << std::string(at, length);
        parser->setError(1001);
        return;
    }
    //解析得到的版本信息设置到 HttpRequest 对象中。
    parser->getData()->setVersion(v);
}

void on_request_header_done(void *data, const char *at, size_t length) {
    //HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
}

//解析 HTTP 请求的字段信息并将其存储在请求对象中，前提是字段名称长度不为零。
void on_request_http_field(void *data, const char *field, size_t flen
                           ,const char *value, size_t vlen) {
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    //如果长度为零，这被认为是一个无效的 HTTP 请求字段
    if(flen == 0) {
        CAPTAIN_LOG_WARN(g_logger) << "invalid http request field length == 0";
        //parser->setError(1002);
        return;
    }
    parser->getData()->setHeader(std::string(field, flen)
                                ,std::string(value, vlen));
}

HttpRequestParser::HttpRequestParser()
    :m_error(0) {
    m_data.reset(new captain::http::HttpRequest);
    http_parser_init(&m_parser);
    m_parser.request_method = on_request_method; //on_xx_xx 回调函数
    m_parser.request_uri = on_request_uri;
    m_parser.fragment = on_request_fragment;
    m_parser.request_path = on_request_path;
    m_parser.query_string = on_request_query;
    m_parser.http_version = on_request_version;
    m_parser.header_done = on_request_header_done;
    m_parser.http_field = on_request_http_field;
    m_parser.data = this; //为了在回调函数中能够访问到当前对象的成员变量和方法，以便进行解析和数据存储操作。
}
//获取 HTTP 请求中的内容长度（Content-Length）
uint64_t HttpRequestParser::getContentLength() {
    //从请求对象中获取 Content-Length 字段的值，如果该字段不存在或解析失败，则返回默认值 0。
    return m_data->getHeaderAs<uint64_t>("content-length", 0);
}

//1: 成功
//-1: 有错误
//>0: 已处理的字节数，且data有效数据为len - v;
size_t HttpRequestParser::execute(char* data, size_t len) {
    size_t offset = http_parser_execute(&m_parser, data, len, 0);
    memmove(data, data + offset, (len - offset));
    return offset;
}

//检查 HTTP 请求是否解析完成
int HttpRequestParser::isFinished() {
    return http_parser_finish(&m_parser);
}

//检查是否存在解析错误
int HttpRequestParser::hasError() {
    return m_error || http_parser_has_error(&m_parser);
}

//处理 HTTP 响应中的状态原因（Reason Phrase）。它将状态原因的字符串存储在响应解析器的数据中。
void on_response_reason(void *data, const char *at, size_t length) {
    HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);
    parser->getData()->setReason(std::string(at, length));
}

//处理 HTTP 响应中的状态码
void on_response_status(void *data, const char *at, size_t length) {
    HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);
    HttpStatus status = (HttpStatus)(atoi(at));
    parser->getData()->setStatus(status);
}

void on_response_chunk(void *data, const char *at, size_t length) {
}

//解析 HTTP 响应中的版本号
void on_response_version(void *data, const char *at, size_t length) {
    // 将数据指针从无类型指针（void*）转换为 HttpResponseParser* 类型
    HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);
    uint8_t v = 0; // 用于存储 HTTP 响应的版本号，初始化为 0
    // 检查 HTTP 响应版本是否为 "HTTP/1.1"
    if(strncmp(at, "HTTP/1.1", length) == 0) {
        v = 0x11;
    } else if(strncmp(at, "HTTP/1.0", length) == 0) {
        v = 0x10;
    } else { // 如果版本号无效，记录警告日志，并设置解析错误码为 1001
        CAPTAIN_LOG_WARN(g_logger) << "invalid http response version: "
            << std::string(at, length);
        parser->setError(1001);
        return;
    }
    // 将解析得到的版本号设置到 HttpResponse 对象中
    parser->getData()->setVersion(v);
}

void on_response_header_done(void *data, const char *at, size_t length) {
}

void on_response_last_chunk(void *data, const char *at, size_t length) {
}

//解析 HTTP 响应中的字段名和字段值，并将它们设置到 HttpResponse 对象的头部中。
void on_response_http_field(void *data, const char *field, size_t flen
                           ,const char *value, size_t vlen) {
    HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);
    if(flen == 0) {
        CAPTAIN_LOG_WARN(g_logger) << "invalid http response field length == 0";
        //parser->setError(1002);
        return;
    }
    parser->getData()->setHeader(std::string(field, flen)
                                ,std::string(value, vlen));
}

HttpResponseParser::HttpResponseParser()
    :m_error(0) {
    m_data.reset(new captain::http::HttpResponse);
    httpclient_parser_init(&m_parser);
    m_parser.reason_phrase = on_response_reason;
    m_parser.status_code = on_response_status;
    m_parser.chunk_size = on_response_chunk;
    m_parser.http_version = on_response_version;
    m_parser.header_done = on_response_header_done;
    m_parser.last_chunk = on_response_last_chunk;
    m_parser.http_field = on_response_http_field;
    m_parser.data = this;
}

//执行HTTP响应的解析
size_t HttpResponseParser::execute(char* data, size_t len, bool chunck) {
    if(chunck) {
        httpclient_parser_init(&m_parser);
    }
    size_t offset = httpclient_parser_execute(&m_parser, data, len, 0);

    memmove(data, data + offset, (len - offset));
    return offset;
}

//检查HTTP响应是否已经解析完成
int HttpResponseParser::isFinished() {
    return httpclient_parser_finish(&m_parser);
}

//检查HTTP响应解析过程中是否有错误
int HttpResponseParser::hasError() {
    return m_error || httpclient_parser_has_error(&m_parser);
}

//获取HTTP响应中的消息体长度
uint64_t HttpResponseParser::getContentLength() {
    return m_data->getHeaderAs<uint64_t>("content-length", 0);
}

}
}
