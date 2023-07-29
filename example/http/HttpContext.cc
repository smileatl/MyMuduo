
#include "HttpContext.h"
#include <mymuduo/Buffer.h>

// 解析和处理 HTTP 请求行，并将解析得到的方法、路径、查询字符串和版本信息设置到 request_ 对象中。这是解析完整的 HTTP 请求的关键步骤之一。
bool HttpContext::processRequestLine(const char* begin, const char* end) {
    // 定义一个布尔类型变量 succeed，初始化为 false
    bool succeed = false;
    // 定义两个指向请求行起始位置和结束位置的指针 start 和
    // end，并将传入的参数赋值给它们
    const char* start = begin;
    // 在请求行中查找第一个空格字符，并将其位置赋值给指针 space
    const char* space = std::find(start, end, ' ');
    // 判断空格字符是否在请求行结束之前，并调用 request_.setMethod(start, space)
    // 函数将方法（请求行中的第一个字段）设置到 request_对象中
    if (space != end && request_.setMethod(start, space)) {
        start = space + 1;
        // 继续查找下一个空格字符
        space = std::find(start, end, ' ');
        // 判断空格字符是否在请求行结束之前
        if (space != end) {
            const char* question = std::find(start, space, '?');
            // 判断空格字符是否在请求行结束之前
            if (question != space) {
                request_.setPath(start, question);
                request_.setQuery(question, space);
            } else {
                // 如果不存在查询字符串，则直接调用 request_.setPath(start, space) 设置路径到 request_ 对象中。
                request_.setPath(start, space);
            }
            start = space + 1;
            // 如果不存在查询字符串，则直接调用 request_.setPath(start, space) 设置路径到 request_ 对象中。
            succeed = end - start == 8 && std::equal(start, end - 1, "HTTP/1.");
            if (succeed) {
                if (*(end - 1) == '1') {
                    // 如果满足条件，将请求的版本设置为 HttpRequest::kHttp11 或 HttpRequest::kHttp10，并将 succeed 设置为 true
                    request_.setVersion(HttpRequest::kHttp11);
                } else if (*(end - 1) == '0') {
                    request_.setVersion(HttpRequest::kHttp10);
                } else {
                    // 如果不满足条件，将 succeed 设置为 false。
                    succeed = false;
                }
            }
        }
    }
    return succeed;
}

// return false if any error
// 逐步解析和处理 HTTP 请求的不同部分，包括请求行、头部字段和正文。它使用 Buffer 类提供的功能来操作缓冲区，同时利用 processRequestLine() 和相关函数来解析和保存请求的相关信息。这是一个典型的基于缓冲区的逐行解析方式。
bool HttpContext::parseRequest(Buffer* buf, Timestamp receiveTime) {
    bool ok = true;
    bool hasMore = true;
    // 循环，直到处理完所有请求部分或出现错误。
    while (hasMore) {
        if (state_ == kExpectRequestLine) {
            // 找到了回车换行的位置
            const char* crlf = buf->findCRLF();
            if (crlf) {
                // 处理请求行，将结果保存到 ok 变量中
                ok = processRequestLine(buf->peek(), crlf);
                // 如果处理成功
                if (ok) {
                    // 设置请求对象的接收时间为 receiveTime。
                    request_.setReceiveTime(receiveTime);
                    // 将已处理的部分从缓冲区中取出
                    buf->retrieveUntil(crlf + 2);
                    // 将状态更新为 kExpectHeaders（期望头部）。
                    state_ = kExpectHeaders;
                } else {
                    hasMore = false;
                }
            } else {
                hasMore = false;
            }
        } else if (state_ == kExpectHeaders) {
            // 当前状态为 kExpectHeaders（期望头部）
            const char* crlf = buf->findCRLF();
            // 当前状态为 kExpectHeaders（期望头部）
            if (crlf) {
                // 调用 std::find() 函数在缓冲区的当前读位置和 crlf 之间查找冒号字符，返回指针colon。
                const char* colon = std::find(buf->peek(), crlf, ':');
                if (colon != crlf) {
                    // 将头部字段添加到请求对象中
                    request_.addHeader(buf->peek(), colon, crlf);
                } else {
                    // 如果相等,说明已经到最后了
                    // 设置状态为 kGotAll（已接收完整请求），并设置 hasMore 为 false，结束循环
                    state_ = kGotAll;
                    hasMore = false;
                }
                // 将已处理的部分从缓冲区中取出
                buf->retrieveUntil(crlf + 2);
            } else {
                hasMore = false;
            }
        } else if (state_ == kExpectBody) {
        }
    }
    return ok;
}
