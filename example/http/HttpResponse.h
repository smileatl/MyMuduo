#pragma once

#include <mymuduo/Buffer.h>

#include <map>

class Buffer;
class HttpResponse {
public:
    // http响应的状态码
    // 200：成功响应
    // 301：永久重定向
    // 400：请求错误
    // 404：资源未找到
    enum HttpStatusCode {
        kUnknown,
        k200Ok = 200,
        k301MovedPermanently = 301,
        k400BadRequest = 400,
        k404NotFound = 404,
    };

    explicit HttpResponse(bool close)
        : statusCode_(kUnknown), closeConnection_(close) {}

    void setStatusCode(HttpStatusCode code) { statusCode_ = code; }

    // 设置状态消息
    void setStatusMessage(const std::string& message) {
        statusMessage_ = message;
    }

    // 设置是否关闭连接
    void setCloseConnection(bool on) { closeConnection_ = on; }

    // 返回当前连接是否需要关闭。该函数没有参数，返回closeConnection_成员变量的值
    bool closeConnection() const { return closeConnection_; }

    // 设置响应的内容类型
    void setContentType(const std::string& contentType) {
        addHeader("Content-Type", contentType);
    }

    // 添加自定义的头部字段和对应的值
    void addHeader(const std::string& key, const std::string& value) {
        headers_[key] = value;
    }

    // 设置响应的正文内容
    void setBody(const std::string& body) { body_ = body; }

    // 将响应的内容追加到缓冲区中
    void appendToBuffer(Buffer* output) const;

private:
    std::map<std::string, std::string> headers_;
    HttpStatusCode statusCode_;

    std::string statusMessage_;
    bool closeConnection_;
    std::string body_;
};
