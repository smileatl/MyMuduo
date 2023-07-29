#pragma once

#include <mymuduo/Timestamp.h>

#include <assert.h>
#include <stdio.h>
#include <map>
#include <string>

class HttpRequest {
public:
    // http请求的方法，kInvalid无效
    enum Method { kInvalid, kGet, kPost, kHead, kPut, kDelete };
    // http请求的版本，包括未知(kUnknown)和HTTP/1.0(kHttp10)以及HTTP/1.1(kHttp11)
    enum Version { kUnknown, kHttp10, kHttp11 };

    // 初始化列表
    HttpRequest() : method_(kInvalid), version_(kUnknown) {}

    void setVersion(Version v) { version_ = v; }

    Version getVersion() const { return version_; }

    bool setMethod(const char* start, const char* end) {
        assert(method_ == kInvalid);
        std::string m(start, end);
        if (m == "GET") {
            method_ = kGet;
        } else if (m == "POST") {
            method_ = kPost;
        } else if (m == "HEAD") {
            method_ = kHead;
        } else if (m == "PUT") {
            method_ = kPut;
        } else if (m == "DELETE") {
            method_ = kDelete;
        } else {
            method_ = kInvalid;
        }
        // 表明设置方法成功
        return method_ != kInvalid;
    }

    Method method() const { return method_; }

    // 用于将一个枚举类型 method_ 转换为对应的字符串表示
    const char* methodString() const {
        const char* result = "UNKNOWN";
        switch (method_) {
            case kGet:
                result = "GET";
                break;
            case kPost:
                result = "POST";
                break;
            case kHead:
                result = "HEAD";
                break;
            case kPut:
                result = "PUT";
                break;
            case kDelete:
                result = "DELETE";
                break;
            default:
                break;
        }
        return result;
    }

    void setPath(const char* start, const char* end) {
        // 两个字符指针，赋给一个字符串path_
        path_.assign(start, end);
    }

    const std::string& path() const { return path_; }

    void setQuery(const char* start, const char* end) {
        query_.assign(start, end);
    }

    const std::string& query() const { return query_; }

    void setReceiveTime(Timestamp t) { receiveTime_ = t; }

    Timestamp receiveTime() const { return receiveTime_; }

    // 用于解析HTTP报文头部中的字段和值，并将其添加到一个存储头部信息的std::map对象headers_中。
    // 三个参数：start、colon和end，分别表示报文头部字段的起始位置、冒号（":"）的位置和结束位置。
    void addHeader(const char* start, const char* colon, const char* end) {
        // field表示报文头部字段的字符串
        std::string field(start, colon);
        // 跳过冒号字符
        ++colon;
        // isspace跳过空格
        while (colon < end && isspace(*colon)) {
            ++colon;
        }
        std::string value(colon, end);
        // 处理可能存在的尾部空白字符，通过缩小value的大小（调用resize()函数），去除尾部的空白字符。
        // value表示报文头部值的字符串
        while (!value.empty() && isspace(value[value.size() - 1])) {
            value.resize(value.size() - 1);
        }
        // 将field和value存入headers_容器中，以字段作为键，对应的值作为值进行存储
        headers_[field] = value;
    }

    // 找到头部字段对应的值
    std::string getHeader(const std::string& field) const {
        std::string result;
        std::map<std::string, std::string>::const_iterator it =
            headers_.find(field);
        if (it != headers_.end()) {
            result = it->second;
        }
        return result;
    }

    const std::map<std::string, std::string>& headers() const {
        return headers_;
    }

    // 用于交换当前HttpRequest对象和另一个HttpRequest对象 (that)的成员变量的值。
    void swap(HttpRequest& that) {
        std::swap(method_, that.method_);
        std::swap(version_, that.version_);
        path_.swap(that.path_);
        query_.swap(that.query_);
        receiveTime_.swap(that.receiveTime_);
        headers_.swap(that.headers_);
    }

private:
    Method method_;
    Version version_;
    std::string path_;
    std::string query_;
    Timestamp receiveTime_;
    std::map<std::string, std::string> headers_;
};
