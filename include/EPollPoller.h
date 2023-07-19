#pragma once

#include "Poller.h"
#include "Timestamp.h"

#include <vector>
#include <sys/epoll.h>

class Channel;

/**
 * 封装epoll的操作行为
 * epoll的使用  
 * epoll_create
 * epoll_ctl   add/mod/del
 * epoll_wait
 */ 

// 基类是抽象类，一定是虚函数
// 派生类覆盖
class EPollPoller : public Poller{
public:
    EPollPoller(EventLoop* loop);
    ~EPollPoller() override; // 基类的析构一定是个虚函数

    // 重写基类Poller的抽象方法
    Timestamp poll(int timeoutMs, ChannelList *activeChannels) override;
    void updateChannel(Channel *channel) override;
    void removeChannel(Channel *channel) override;


private:
    static const int kInitEventListSize = 16;

    // 填写活跃的连接
    void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;
    // 更新channel通道
    void update(int operation, Channel *channel);

    using EventList=std::vector<epoll_event>;

    // create创建时的fd，释放的时候再释放掉
    int epollfd_;
    EventList events_;

};