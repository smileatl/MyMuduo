#pragma once
#include "noncopyable.h"

#include <functional>
#include <string>
#include <vector>
#include <memory>

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool:noncopyable{
public:
    using ThreadInitCallback=std::function<void(EventLoop*)>;

    // baseLoop事件循环
    EventLoopThreadPool(EventLoop *baseLoop, const std::string &nameArg);
    ~EventLoopThreadPool();

    void setThreadNum(int numThreads){
        numThreads_=numThreads;
    }

    void start(const ThreadInitCallback &cb=ThreadInitCallback());

    // 如果工作在多线程中，baseLoop_默认以轮询的方式分配channel给subloop
    EventLoop* getNextLoop();

    // 返回池里所有的loops
    std::vector<EventLoop*> getAllLoops();

    bool started() const { return started_; }
    const std::string name() const { return name_; }

private:
    EventLoop *baseLoop_; // EventLoop loop;  
    std::string name_;
    bool started_;
    int numThreads_;
    int next_; //做下一个loop的下表用的；
    // 包含所有创建的事件的线程
    std::vector<std::unique_ptr<EventLoopThread>> threads_;
    // 包含事件线程里，所有EventLoop的指针
    // 通过调用EventLoopThread中startLoop()可以产生
    std::vector<EventLoop*> loops_;


};