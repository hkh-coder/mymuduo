#include "Thread.h"
#include "CurrentThread.h"
#include <semaphore.h>

std::atomic_int Thread::numCreated_(0);

Thread::Thread(ThreadFunc func,const std::string &name)
    : started_(false)
    , joined_(false)
    , tid_(0)
    , func_(std::move(func))
    , name_(name)
{
    setDefultName();
}

Thread::~Thread()
{
    if (started_ && !joined_)
    { // 设置线程分离(守护线程)，当主线程结束时，子线程也退出
        threadId_->detach();
    }
}
// 启动线程
void Thread::start()
{
    started_ = true;
    // 使用一个信号量，确保线程tid值已经获取到
    sem_t sem;
    sem_init(&sem,false,0);

    // 使用一个lambda然后获取外部变量[&],这样这个新的线程就可以访问当前Thread对象的资源
    // 从而将新创建的thread和Thread对象联系在一起
    threadId_ = std::shared_ptr<std::thread>(new std::thread([&](){
        // 获取当前线程的tid
        tid_ = CurrentThread::tid();
        sem_post(&sem); // 信号量加1

        func_(); // 执行线程函数
    }));

    sem_wait(&sem);
}

// 设置join
void Thread::join()
{
    joined_ = true;
    threadId_->join();
}

// 给线程设置一个默认的名字
void Thread::setDefultName()
{
    int num = ++numCreated_;
    if (name_.empty())
    {
        char buf[32] = {0};
        snprintf(buf, sizeof buf, "Thread%d", num);
        name_ = buf;
    }
}