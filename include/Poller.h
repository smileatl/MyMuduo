#pragma once 
#include "noncopyable.h"
#include <vector>

#include "noncopyable.h"
#include "Timestamp.h"

#include <vector>
#include <unordered_map>

class Channel;
class EventLoop;

// muduo库中多路事件分发器的核心IO复用模块
class Poller:noncopyable{
public: 
    using ChannelList=std::vector<Channel*>;

    Poller(EventLoop *loop);
    // 虚析构函数
    // 如果基类的析构函数不是虚函数，那么当通过基类指针或引用删除派生类对象时，
    // 只会调用基类的析构函数，而不会调用派生类的析构函数。
    // 这可能导致派生类中的资源没有得到正确释放，造成内存泄漏或其他问题。
    virtual ~Poller()=default;

    // 纯虚函数，抽象类不能实例
    // 给所有IO复用保留统一的接口,需要它们去重写的;
    //当前激活的channel，运行的channel，需要Poller去照顾的channel都在ChannelList里边;
    virtual Timestamp poll(int timeouts, ChannelList *activeChannels)=0;
    virtual void updateChannel(Channel *channel)=0;
    virtual void removeChannel(Channel *channel)=0;

    // 判断参数channel是否在当前的Poller当中
    bool hasChannel(Channel *channel) const;

    // EventLoop可以通过该接口获取默认的IO复用的具体实现
    static Poller* newDefaultPoller(EventLoop *loop);

protected:
    // map的key：sockfd  value：sockfd所属的channel通道类型
    using ChannelMap=std::unordered_map<int, Channel*>;
    //poller监听的channel是EventLoop（EventLoop里边有ChannelList）里边保存的channel
    ChannelMap channels_;
    
private:
    EventLoop *ownerLoop_; // 定义Poller所属的事件循环EventLoop
};