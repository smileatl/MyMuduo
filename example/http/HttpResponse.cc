
#include "HttpResponse.h"
#include <mymuduo/Buffer.h>

#include <stdio.h>

// 用于将HTTP响应的内容追加到指定的缓冲区中
void HttpResponse::appendToBuffer(Buffer* output) const {
    char buf[32];
    // 创建一个大小为 32 的字符数组 buf，并使用 snprintf()
    // 函数将状态码转换为字符串形式，并拼接到输出缓冲区 output 中
    snprintf(buf, sizeof buf, "HTTP/1.1 %d ", statusCode_);
    output->append(buf);
    // 将响应消息的状态描述符（例如 "OK"、"Not Found"）拼接到输出缓冲区output中
    output->append(statusMessage_);
    // 拼接回车换行符 "\r\n" 到输出缓冲区 output 中，表示一条头部字段的结束。
    output->append("\r\n");

    // 根据连接状态 closeConnection_ 的值，选择性地添加 Connection
    // 头部字段。如果连接需要关闭，则添加 "Connection: close\r\n"
    // 到输出缓冲区output 中
    if (closeConnection_) {
        output->append("Connection: close\r\n");
    } else {
        // 否则，根据响应正文的长度动态计算并添加 "Content-Length" 和
        // "Connection: Keep-Alive\r\n" 到输出缓冲区 output 中
        snprintf(buf, sizeof buf, "Content-Length: %zd\r\n", body_.size());
        output->append(buf);
        output->append("Connection: Keep-Alive\r\n");
    }

    // 遍历 headers_ 容器中的每个头部字段和对应的值，依次将它们拼接到输出缓冲区 output 中
    for (const auto& header : headers_) {
        output->append(header.first);
        output->append(": ");
        output->append(header.second);
        output->append("\r\n");
    }

    // 添加额外的回车换行符 "\r\n" 到输出缓冲区 output 中，表示头部结束
    output->append("\r\n");
    // 将响应正文 body_ 追加到输出缓冲区 output 中。
    output->append(body_);
}
