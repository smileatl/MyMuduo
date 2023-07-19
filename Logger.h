#pragma once

#include <string>
#include "noncopyable.h"

// 定义宏
// LOG_INFO("%s %d", arg1, arg2)
// 用宏来接受可变参, 定义大宏的时候都会用do while

// 将可变参数__VA_ARGS__根据格式字符串logmsgFormat进行格式化，
// 它的作用是在可变参数为空的情况下，省略前面的逗号。这样可以避免在宏展开时出现语法错误。
#define LOG_INFO(logmsgFormat, ...)                       \
    do {                                                  \
        Logger& logger = Logger::instance();              \
        logger.setLogLevel(INFO);                         \
        char buf[1024] = {0};                             \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf);                                  \
    } while (0)

#define LOG_ERROR(logmsgFormat, ...)                      \
    do {                                                  \
        Logger& logger = Logger::instance();              \
        logger.setLogLevel(ERROR);                        \
        char buf[1024] = {0};                             \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf);                                  \
    } while (0)

#define LOG_FATAL(logmsgFormat, ...)                      \
    do {                                                  \
        Logger& logger = Logger::instance();              \
        logger.setLogLevel(FATAL);                        \
        char buf[1024] = {0};                             \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf);                                  \
        exit(-1);                                         \
    } while (0)

// 调试时IO操作
// 如果自己定义了MUDEBUG
#ifdef MUDEBUG
#define LOG_DEBUG(logmsgFormat, ...)                      \
    do {                                                  \
        Logger& logger = Logger::instance();              \
        logger.setLogLevel(DEBUG);                        \
        char buf[1024] = {0};                             \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf);                                  \
    } while (0)
#else
// 否则就什么也不用输出
#define LOG_DEBUG(logmsgFormat, ...)
#endif

// 定义日志的级别 INFO RERROR FATAL（毁灭性错误） DEBUG（可选开关）
enum LogLevel {
    INFO,   // 普通信息
    ERROR,  // 错误信息
    FATAL,  // core信息
    DEBUG,  // 调试信息
};

// 输出一个日志类，单例就好，不需要进行拷贝构造和赋值
class Logger : noncopyable {
public:
    // 获取日志唯一的实例对象
    // 用引用&，这样调用方法的时候方便，可以用.
    static Logger& instance();
    // 设置日志级别
    void setLogLevel(int level);
    // 写日志
    void log(std::string msg);

private:
    // _logLevel; 和系统的变量不产生冲突
    int logLevel_;
};