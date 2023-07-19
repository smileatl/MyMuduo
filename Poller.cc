#include "Poller.h"
#include "Channel.h"

Poller::Poller(EventLoop* loop):ownerLoop_(loop){}

bool Poller::hasChannel(Channel *channel) const
{
    auto it = channels_.find(channel->fd());
    // 不等于存在
    return it != channels_.end() && it->second == channel;
}