#include "Channel.h"

#include <sys/epoll.h>
#include "EventLoop.h"
#include "Logger.h"

// 类静态成员需要在类外初始化
const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

// EventLoop: ChannelList Poller
Channel::Channel(EventLoop* loop, int fd)
    : loop_(loop), fd_(fd), events_(0), revents_(0), index_(-1), tied_(false) {}

Channel::~Channel() {
    // // 在原库里做了断言，我们这里可以什么都不做
    // assert(!eventHandling_);
    // assert(!addedToLoop_);
    // if (loop_->isInLoopThread()) {
    //     // eventloop释放的必须是自己的channel
    //     assert(!loop_->hasChannel(this));
    // }
}

// 强弱、智能指针的应用需要学
// 一个TcpConnection新连接创建的时候 TcpConnection => Channel 
// channel的tie方法什么时候调用过？
// 有一个强智能指针记着，资源就不会被释放
void Channel::tie(const std::shared_ptr<void>& obj) {
    tie_ = obj;
    tied_ = true;
}

/**
 * 当改变channel所表示fd的events事件后，update负责在poller里面更改fd相应的事件epoll_ctl
 * channel需要通过eventloop才能控制poller
 * EventLoop => ChannelList   Poller
 */
void Channel::update() {
    // 通过channel所属的eventloop，调用poller的相应方法，注册fd的events事件
    // add code...
    loop_->updateChannel(this); // this就是channel
}

// 在channel中所属的eventloop中，把当前的channel删除掉
void Channel::remove() {
    // 断言不需要
    // add code
    loop_->removeChannel(this);
}

// fd得到poller通知以后，处理事件的
void Channel::handleEvent(Timestamp receiveTime) {
    if (tied_) {
        std::shared_ptr<void> guard = tie_.lock();
        if (guard) {
            handleEventWithGuard(receiveTime);
        }
    } else {
        handleEventWithGuard(receiveTime);
    }
}

// 根据poller通知的channel发生的具体事件， 由channel负责调用具体的回调操作
void Channel::handleEventWithGuard(Timestamp receiveTime) {
    // __FUNCTION__   __LINE__ 内带的宏，打印函数、行号
    // 打印日志信息
    LOG_INFO("channel handleEvent revents:%d\n", revents_);

    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) {
        // 发生异常了
        if (closeCallback_) {
            closeCallback_();
        }
    }

    if (revents_ & EPOLLERR) {
        if (errorCallback_) {
            errorCallback_();
        }
    }

    if (revents_ & (EPOLLIN | EPOLLPRI)) {
        if (readCallback_) {
            readCallback_(receiveTime);
        }
    }

    if (revents_ & EPOLLOUT) {
        if (writeCallback_) {
            writeCallback_();
        }
    }
}