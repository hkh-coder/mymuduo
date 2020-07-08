#include "EventLoopThread.h"
#include "EventLoop.h"

EventLoopThread::EventLoopThread(const ThreadInitCallback &cb, const std::string &name)
    : loop_(nullptr)
    , exiting_(false)
    , thread_(std::bind(&EventLoopThread::threadFunc,this),name)
    , callback_(cb)
    , mutex_()
    ,cond_()
{

}
EventLoopThread::~EventLoopThread()
{
    exiting_ = true;
    if(loop_ != nullptr)
    {   // loop_不为空，先将loop停止，然后再退出线程
        loop_->quit();
        thread_.join();
    }
}

EventLoop *EventLoopThread::startLoop()
{   
    thread_.start(); // 启动一个新线程
    
    EventLoop *loop = nullptr;
    
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while(loop_ == nullptr)
        {
            // wait是当前线程等待新的线程调用threadFunc
            // 并且得到通知，然后继续向后运行
            cond_.wait(lock);
        }
        loop = loop_;
    }
    
    return loop;
}

// 新线程调用的线程函数
void EventLoopThread::threadFunc()
{
    // 启动新线程创建一个loop --> per thread one loop
    EventLoop loop;
    if(callback_)
    {
        callback_(&loop);
    }

    {
        std::unique_lock<std::mutex> lock(mutex_);
        loop_ = &loop;
        cond_.notify_one();
    }
    loop.loop(); // 启动poller的循环

    // EventLoop退出
    std::unique_lock<std::mutex> lock(mutex_);
    loop_ = nullptr;
}
