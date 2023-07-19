#include "Poller.h"
#include "EPollPoller.h"

#include <stdlib.h>

// DefaultPoller.cc属于一个poller公共的源文件
// 分解了poller和具体的epollpoller
// 派生类依赖基类，基类不要依赖派生类，基类本身是抽象的，不能依赖具体的实现

// Poller.h属于最上层基类层的，最后去引用派生类相关的实现
Poller* Poller::newDefaultPoller(EventLoop *loop){
    // 全局函数，没有任何命名空间
    if (::getenv("MUDUO_USE_POLL"))
    {
        return nullptr; // 生成poll的实例
    }
    else
    {
        // 默认生成epoll复用实例
        return new EPollPoller(loop); // 生成epoll的实例
    }
} 