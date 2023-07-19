#include "EPollPoller.h"
#include "Channel.h"
#include "Logger.h"

#include <errno.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

// index_表示channel在poller中的状态
// channel未添加到poller中
const int kNew = -1;  // channel的成员index_ = -1
// channel已添加到poller中
const int kAdded = 1;
// channel从poller中删除
const int kDeleted = 2;

EPollPoller::EPollPoller(EventLoop* loop)
    : Poller(loop),
      epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
      events_(kInitEventListSize)  // vector<epoll_event>
{
    if (epollfd_ < 0) {
        LOG_FATAL("epoll_create error:%d \n", errno);
    }
}

EPollPoller::~EPollPoller() {
    ::close(epollfd_);
}

Timestamp EPollPoller::poll(int timeoutMs, ChannelList* activeChannels) {
    // 实际上应该用LOG_DEBUG输出日志更为合理
    LOG_INFO("func=%s => fd total count:%lu \n", __FUNCTION__,
             channels_.size());

    // &*events_.begin()数组的其实地址
    int numEvents = ::epoll_wait(epollfd_, &*events_.begin(),
                                 static_cast<int>(events_.size()), timeoutMs);
    int saveErrno = errno;

    Timestamp now(Timestamp::now());

    if (numEvents > 0) {
        // 发生事件的个数
        LOG_INFO("%d events happened \n", numEvents);
        fillActiveChannels(numEvents, activeChannels);
        if (numEvents == events_.size()) {
            events_.resize(events_.size() * 2);
        }
    } else if (numEvents == 0) {
        LOG_DEBUG("%s timeout! \n", __FUNCTION__);
    } else {
        if (saveErrno != EINTR) {
            errno = saveErrno;
            LOG_ERROR("EPollPoller::poll() err!");
        }
    }
    // 返回具体发生事件的事件点
    return now;
}

// channel update remove => EventLoop updateChannel removeChannel => Poller
// updateChannel removeChannel
/**
 *              EventLoop  =>
 * poller.poll通过epoll_wait把真正发生事件的channel，通过形参填充到EventLoop提供的实参里
 *     ChannelList      Poller(是通过其抽象基类表示的)
 *                     ChannelMap  <fd, channel*>   epollfd
 *                  向poller里注册过的，对ChannelMap[epollfd]操作
 *     ChannelList >= ChannelMap
 */
void EPollPoller::updateChannel(Channel* channel) {
    const int index = channel->index();
    // 体现具体的函数
    LOG_INFO("func=%s => fd=%d events=%d index=%d \n", __FUNCTION__,
             channel->fd(), channel->events(), index);

    // 未添加或者已删除
    if (index == kNew || index == kDeleted) {
        if (index == kNew) {
            // 把fd相关的事件进行更改
            int fd = channel->fd();
            channels_[fd] = channel;
        }
        // 设置index为已添加
        channel->set_index(kAdded);
        // 然后注册到poller里面
        update(EPOLL_CTL_ADD, channel);
    } else  // channel已经在poller上注册过了
    {
        int fd = channel->fd();
        if (channel->isNoneEvent()) {
            // 已经无事件了，删除
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);
        } else {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

// 从poller中删除channel
// 就是从Poller中的ChannelMap删除掉
void EPollPoller::removeChannel(Channel* channel) {
    int fd = channel->fd();
    channels_.erase(fd);

    LOG_INFO("func=%s => fd=%d\n", __FUNCTION__, fd);

    int index = channel->index();
    if (index == kAdded) {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(kNew);
}

// 填写活跃的连接
void EPollPoller::fillActiveChannels(int numEvents,
                                     ChannelList* activeChannels) const {
    for (int i = 0; i < numEvents; ++i) {
        // channel是事件中的data.ptr
        Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        activeChannels->push_back(
            channel);  // EventLoop就拿到了它的poller给它返回的所有发生事件的channel列表了
    }
}

// 更新channel通道 epoll_ctl add/mod/del
// 此函数的作用是在epoll实例中注册或更新指定文件描述符上的事件。具体的操作类型和事件类型由调用者根据需要决定。
// operation：操作类型，可以是EPOLL_CTL_ADD、EPOLL_CTL_MOD或EPOLL_CTL_DEL，分别表示添加、修改和删除事件。
// channel：要更新事件的通道对象。
void EPollPoller::update(int operation, Channel* channel) {
    epoll_event event;
    bzero(&event, sizeof event);

    // 从channel对象中获取文件描述符fd
    int fd = channel->fd();

    // 将channel对象的事件类型（通过events()方法获取）赋值给event.events字段
    event.events = channel->events();
    // 将文件描述符fd赋值给event.data.fd字段
    // 拿到了fd，就可以根据fd找到对应的channel
    event.data.fd = fd;
    // 将channel对象的指针赋值给event.data.ptr字段，以便在事件发生时可以通过该指针访问相关的通道对象。
    event.data.ptr = channel;

    // 调用epoll_ctl函数进行事件的添加、修改或删除操作。如果操作失败，根据不同的操作类型输出相应的日志信息。
    if (::epoll_ctl(epollfd_, operation, fd, &event) < 0) {
        if (operation == EPOLL_CTL_DEL) {
            LOG_ERROR("epoll_ctl del error:%d\n", errno);
        } else {
            LOG_FATAL("epoll_ctl add/mod error:%d\n", errno);
        }
    }
}