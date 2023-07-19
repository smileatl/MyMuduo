#include "Thread.h"
#include "CurrentThread.h"

#include <semaphore.h>

// 类的静态成员变量需要在类外单独初始化
// ()标准的指定构造函数
std::atomic_int Thread::numCreated_(0);

// 参数的默认值只出现一个地方就行了
Thread::Thread(ThreadFunc func, const std::string &name)
    : started_(false)
    , joined_(false)
    , tid_(0)
    , func_(std::move(func))
    , name_(name)
{
    setDefaultName();
}

Thread::~Thread()
{
    if (started_ && !joined_)
    {
        thread_->detach(); // thread类提供的设置分离线程的方法
    }
}

// 一个Thread对象，记录的就是一个新线程的详细信息
void Thread::start(){
    started_ = true;
    sem_t sem;
    sem_init(&sem, false, 0);
    // 开启线程
    // 相当于产生了一个线程对象，传一个线程函数
    thread_ = std::shared_ptr<std::thread>(new std::thread([&](){
        // 获取线程的tid值
        tid_ = CurrentThread::tid();
        sem_post(&sem);
        // 开启一个新线程，专门执行该线程函数
        // 子线程
        func_(); 
    }));

    // 这里必须等待上面新创建的线程的tid值
    sem_wait(&sem);
}

// 创建默认的线程
void Thread::setDefaultName(){
    int num=++numCreated_;
    if(name_.empty()){
        char buf[32]={0};
        snprintf(buf, sizeof buf, "Thread%d", num);
        name_=buf;
    }
}

void Thread::join()
{
    joined_ = true;
    thread_->join();
}
