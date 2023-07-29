#include "EventLoop.h"
#include "Channel.h"
#include "Logger.h"
#include "Poller.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <memory>

// 防止一个线程创建多个EventLoop
// __thread: thread_local一个线程只能有一个EventLoop
__thread EventLoop* t_loopInThisThread = nullptr;

// 定义默认的Poller IO复用接口的超时时间
const int kPollTimeMs = 10000;

// 创建wakeupfd，用来notify唤醒subReactor处理新来的channel
int createEventfd() {
    //eventfd创建失败，一般不会失败，除非一个进程把文件描述符（Linux一个进程1024个最多）全用光了。
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0) {
        LOG_FATAL("eventfd error:%d \n", errno);
    }
    return evtfd;
}

// wakeupFd_(createEventfd())生成一个生成一个eventfd，每个EventLoop对象，都会有自己的eventfd
EventLoop::EventLoop()
    : looping_(false),
      quit_(false),
      callingPendingFunctors_(false),
      threadId_(CurrentThread::tid()),
      poller_(Poller::newDefaultPoller(this)),
      wakeupFd_(createEventfd()),
      wakeupChannel_(new Channel(this, wakeupFd_)) {
    LOG_DEBUG("EventLoop created %p in thread %d \n", this, threadId_);
    //如果当前线程已经绑定了某个EventLoop对象了，那么该线程就无法创建新的EventLoop对象了
    if (t_loopInThisThread) {
        LOG_FATAL("Another EventLoop %p exists in this thread %d \n",
                  t_loopInThisThread, threadId_);
    } else {
        t_loopInThisThread = this;
    }

    // 设置wakeupfd的事件类型以及发生事件后的回调操作
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    // 每一个eventloop都将监听wakeupchannel的EPOLLIN读事件了
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop() {
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread = nullptr;
}

// 开启事件循环
void EventLoop::loop() {
    looping_ = true;
    quit_ = false;

    LOG_INFO("EventLoop %p start looping \n", this);

    while (!quit_) {
        activeChannels_.clear();
        // 监听两类fd，一种是client的fd，一种wakeupfd
        //此时activeChannels_已经填好了事件发生的channel
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
        for (Channel* channel : activeChannels_) {
            // Poller监听哪些channel发生事件了，然后上报给EventLoop，通知channel处理相应的事件
            channel->handleEvent(pollReturnTime_);
        }
        // 执行当前EventLoop事件循环需要处理的回调操作
        /**
         * mainloop只做accept
         * IO线程 mainLoop accept fd《=channel subloop
         * mainLoop 事先注册一个回调cb（需要subloop来执行）    
         * wakeup subloop后，执行下面的方法，执行之前mainloop注册的cb操作
         */
        doPendingFunctors();
    }
    LOG_INFO("EventLoop %p stop looping. \n", this);
    looping_ = false;
}
// 退出事件循环  1.loop在自己的线程中调用quit  2.在非loop的线程中，调用loop的quit    
/**
 *              mainLoop
 *
 *                                 no
 * ====================生产者-消费者的线程安全的队列（muduo库没有）
 *
 *  subLoop1     subLoop2     subLoop3
 */
// 一个loop是一个线程
void EventLoop::quit() {
    quit_ = true;

    // 如果是在其他线程中，调用的quit
    // 在一个subloop(woker)中，调用了mainloop(IO)的quit
    if (!isInLoopThread()) {
        // 退出其他的loop，需要先把该loop唤醒，再quit
        wakeup();
    }
}

// 在当前loop中执行cb
// 该函数保证了cb这个函数对象一定是在其EventLoop线程中被调用。
void EventLoop::runInLoop(Functor cb) {
    if (isInLoopThread()) {  // 在当前的loop线程中，执行cb
        cb();
    } else {  // 在非当前线程中执行cb，就需要唤醒loop所在线程，执行cb
        queueInLoop(cb);
    }
}
// 把cb放入队列中，唤醒loop所在的线程，执行cb
void EventLoop::queueInLoop(Functor cb) {
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(cb);
    }

    // 唤醒相应的线程，需要执行上面回调操作的loop的线程了
    // ||callingPendingFunctors_的意思是：当前loop正在执行回调，但是loop又有了新的回调，继续唤醒执行新的回调
    if (!isInLoopThread() || callingPendingFunctors_) {
        wakeup();  // 唤醒loop所在线程
    }
}

// 用来唤醒loop所在的线程的
// 向wakeupfd_写一个数据，wakeupChannel就发生读事件，当前loop线程就会被唤醒
void EventLoop::wakeup() {
    uint64_t one = 1;
    ssize_t n = write(wakeupFd_, &one, sizeof one);
    if (n != sizeof one) {
        LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8 \n", n);
    }
}

// EventLoop的方法 =》 Poller的方法
// 这些本来是channel向poller问的，但是channel跟poller无法直接沟通
// 那么channel根据EventLoop跟poller沟通
void EventLoop::updateChannel(Channel* channel) {
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel) {
    poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel* channel) {
    return poller_->hasChannel(channel);
}

void EventLoop::handleRead() {
    uint64_t one = 1;
    ssize_t n = read(wakeupFd_, &one, sizeof one);
    if (n != sizeof one) {
        LOG_ERROR("EventLoop::handleRead() reads %lu bytes instead of 8", n);
    }
}

// 执行回调
void EventLoop::doPendingFunctors() {
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;

    {
        std::unique_lock<std::mutex> lock(mutex_);
        // 相当于把pendingFunctors_解放了
        // 还没执行完当前这个loop执行的回调，
        // 也不妨碍mainloop向当前这个loop写回调
        // 因为pendingFunctors_被释放了
        functors.swap(pendingFunctors_);
    }

    for (const Functor& functor : functors) {
        functor();  // 执行当前loop需要执行的回调操作
    }

    callingPendingFunctors_ = false;
}