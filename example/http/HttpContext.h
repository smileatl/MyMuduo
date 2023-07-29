#pragma once

#include "HttpRequest.h"

class Buffer;

// 用于解析和处理HTTP请求
class HttpContext {
public:
    // 定义了解析请求的几个状态
    // kExpectRequestLine（期望请求行）、kExpectHeaders（期望头部）、kExpectBody（期望正文）、kGotAll（已接收完整请求）
    enum HttpRequestParseState {
        kExpectRequestLine,
        kExpectHeaders,
        kExpectBody,
        kGotAll,
    };

    HttpContext() : state_(kExpectRequestLine) {}

    // default copy-ctor, dtor and assignment are fine
    // return false if any error
    // 解析请求的函数。接受一个指向 Buffer 对象的指针 buf
    // 和一个时间戳receiveTime，并根据缓冲区中的数据解析出一个完整的请求。返回值表示解析是否成功。
    bool parseRequest(Buffer* buf, Timestamp receiveTime);

    // 判断是否已经接收到完整的请求
    bool gotAll() const { return state_ == kGotAll; }

    // 重置对象的状态，将 state_ 设置为 kExpectRequestLine，并通过交换 request_
    // 对象来清空之前的请求内容
    void reset() {
        state_ = kExpectRequestLine;
        HttpRequest dummy;
        request_.swap(dummy);
    }

    // 获取当前解析得到的请求对象的常引用。
    const HttpRequest& request() const { return request_; }

    // 获取当前解析得到的请求对象的引用。
    HttpRequest& request() { return request_; }

private:
    // 处理请求行
    bool processRequestLine(const char* begin, const char* end);

    HttpRequestParseState state_;
    HttpRequest request_;
};
