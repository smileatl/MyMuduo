#pragma once

#include <iostream>
#include <string>

// 时间类
// 只写了需要用的方法
class Timestamp {
public:
    Timestamp();
    // 必须建立对象，显式；不允许隐式转换
    explicit Timestamp(int64_t microSecondsSinceEpoch);
    static Timestamp now();
    std::string toString() const;

    void swap(Timestamp& that) {
        std::swap(microSecondsSinceEpoch_, that.microSecondsSinceEpoch_);
    }

private:
    int64_t microSecondsSinceEpoch_;
};