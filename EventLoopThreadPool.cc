#include "EventLoopThreadPool.h"
#include "EventLoopThread.h"

#include <memory>

EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLoop,
                                         const std::string& nameArg)
    : baseLoop_(baseLoop),
      name_(nameArg),
      started_(false),
      numThreads_(0),
      next_(0) {}

EventLoopThreadPool::~EventLoopThreadPool() {}

// 参数的默认值只需要声明时写就行
void EventLoopThreadPool::start(const ThreadInitCallback& cb) {
    started_ = true;

    // 如果setThreadNum，就会进这个循环
    for (int i = 0; i < numThreads_; ++i) {
        char buf[name_.size() + 32];
        snprintf(buf, sizeof buf, "%s%d", name_.c_str(), i);
        EventLoopThread* t = new EventLoopThread(cb, buf);
        // 根据开启的线程数开启相应的线程，
        threads_.push_back(std::unique_ptr<EventLoopThread>(t));
        //执行startLoop函数，返回loop指针，（没执行threadFunc灰调函数的话，线程会阻塞）
        loops_.push_back(
            t->startLoop());  // 底层创建线程，绑定一个新的EventLoop，并返回该loop的地址
    }

    // 整个服务端只有一个线程，运行这baseloop
    if (numThreads_ == 0 && cb) {
        cb(baseLoop_); //cb(ThreadInitCallback)（用户提前设置的回调）
    }
}

// 如果工作在多线程中，baseLoop_默认以轮询的方式分配channel给subloop
EventLoop* EventLoopThreadPool::getNextLoop() {
    // baseLoop_用户创建的loop，
    // 如果没有setThreadNum()，永远用的都是base_loop
    EventLoop* loop = baseLoop_;

    if (!loops_.empty()) {  // 通过轮询获取下一个处理事件的loop
        loop = loops_[next_];
        ++next_;
        if (next_ >= loops_.size()) {
            next_ = 0;
        }
    }
    return loop;
}

std::vector<EventLoop*> EventLoopThreadPool::getAllLoops() {
    if (loops_.empty()) {
        return std::vector<EventLoop*>(1, baseLoop_);
    } else {
        return loops_;
    }
}

