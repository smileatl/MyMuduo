#pragma once

#include "Timestamp.h"
#include "noncopyable.h"

#include <functional>
#include <memory>

// 类型的前置声明，不要暴露太多头文件中的内容
// EventLoop包含了Poller和Channel
// Channel里包含了fd，events，revents

// 这些事件最终向poller里注册，发生的事件由poller向channel进行通知
// channel得到相应fd的事件通知以后，最终调用相应的回调函数
class EventLoop;
class Timestamp;

/**
 * 理清楚  EventLoop、Channel、Poller之间的关系 《= 在Reactor模型上对应 Demultiplex
 * Channel 理解为通道，封装了sockfd和其感兴趣的event，如EPOLLIN、EPOLLOUT事件
 * 还绑定了poller返回的具体事件
 */ 

class Channel : noncopyable {
public:
    // 回调
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(Timestamp)>;

    Channel(EventLoop* looP, int fd);
    ~Channel();

    // fd得到poller通知以后，处理事件的。就是调用相应的回调方法
    void handleEvent(Timestamp receiveTime);

    // 设置回调函数对象
    // std::move把一个左值转换成右值
    void setReadCallback(ReadEventCallback cb) {
        readCallback_ = std::move(cb);
    }
    void setWriteCallback(EventCallback cb) { writeCallback_ = std::move(cb); }
    void setCloseCallback(EventCallback cb) { closeCallback_ = std::move(cb); }
    void setErrorCallback(EventCallback cb) { errorCallback_ = std::move(cb); }

    // 防止当channel被手动remove掉，channel还在执行回调操作
    // 智能指针
    void tie(const std::shared_ptr<void>&);

    int fd() const { return fd_; }
    int events() const { return events_; }
    // 设置具体发生的事件
    void set_revents(int revt) { revents_ = revt; }

    // 设置fd相应的事件状态
    // 相应的readevent置位
    void enableReading() {
        events_ |= kReadEvent;
        update();
    }
    // 取反后与，相当于取消置位
    void disableReading() {
        events_ &= ~kReadEvent;
        update();
    }
    void enableWriting() {
        events_ |= kWriteEvent;
        update();
    }
    void disableWriting() {
        events_ &= ~kWriteEvent;
        update();
    }
    // 所有事件都取消置位
    void disableAll() {
        events_ = kNoneEvent;
        update();
    }

    // 返回fd当前的事件状态
    bool isNoneEvent() const { return events_ == kNoneEvent; }
    // 相与后还为1，说明确实有这个事件
    bool isWriting() const { return events_ & kWriteEvent; }
    bool isReading() const { return events_ & kReadEvent; }

    int index() { return index_; }
    void set_index(int idx) { index_ = idx; }

    // one loop per thread
    EventLoop* ownerLoop() {return loop_;}
    void remove();

private:
    void update();
    // 用于处理事件，并提供异常保护
    void handleEventWithGuard(Timestamp receiveTime);

    static const int kNoneEvent; // 无事件
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop* loop_;  // 事件循环
    const int fd_;     // fd, Poller监听的对象  epoll_ctrl
    int events_;       // 注册fd感兴趣的事件
    int revents_;      // poller返回的具体发生的事件
    int index_; 

    std::weak_ptr<void> tie_;
    bool tied_;

    // 因为channel通道里面能够获知fd最终发生的具体的事件revents，所以它负责调用具体事件的回调操作
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
};
