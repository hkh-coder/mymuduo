#include "EventLoop.h"
#include "Channel.h"
#include "Logger.h"
#include "Poller.h"

#include <fcntl.h>
#include <memory>
#include <sys/eventfd.h> // eventfd
#include <unistd.h>
#include <errno.h>

// 防止一个线程创建多个EventLoop __thread == thread_local 表示每个线程都有一个EventLoop
__thread EventLoop *t_loopInThisThread = nullptr;

// 定义默认pollerIO复用的超时时间 --> epoll_wait
const int KPollTimeMs = 10000;

// 创建wakeupfd，用来mainloop 唤醒subloop使用
int createEventfd()
{
    // eventfd
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0)
    {
        LOG_FATAL("%s:%s:%d  eventfd error : %d \n"
                  ,__FILE__,__FUNCTION__,__LINE__ ,errno);
    }
    return evtfd;
}

EventLoop::EventLoop()
    : looping_(false)
    , quit_(false)
    , callingPendingFunctors_(false)
    , threadId_(CurrentThread::tid())
    , poller_(Poller::newDefaultPoller(this))
    , wakeupFd_(createEventfd())
    , wakeupChannel_(new Channel(this, wakeupFd_))
{
    LOG_DEBUG("%s:%s:%d  EventLoop created %p in thread %d \n"
                ,__FILE__,__FUNCTION__,__LINE__, this, threadId_);
    if (t_loopInThisThread)
    { // 如果当前线程已经有一个EventLoop就不在创建
        LOG_FATAL("%s:%s:%d  Another EventLoop %p exists in this thread %d \n"
                    ,__FILE__,__FUNCTION__,__LINE__, t_loopInThisThread, threadId_);
    }
    else
    { // 创建一个EventLoop
        t_loopInThisThread = this;
    }

    /**
     * 设置wakeupChannel从而保证每个EventLoop线程都会关注wakeupfd
     * 从而保证了所有的EventLoop之间可以通过wakeupfd来相互通信
     * */

    // 设置wakeupfd发生事件时的回调函数
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    // 设置wakerpfd关注的事件类型(可读)
    wakeupChannel_->enableReading();
}
EventLoop::~EventLoop()
{
    wakeupChannel_->disableAll(); // 取消wakeupfd关注的事件(可读)
    wakeupChannel_->remove();     // 从poller中删除wakeupfd对应的wakeupChannel；
    ::close(wakeupFd_);           // 关闭wakeupfd
    t_loopInThisThread = nullptr;
}

// 启动EventLoop
void EventLoop::loop()
{
    looping_ = true;
    quit_ = false;

    LOG_INFO("%s:%s:%d  EventLoop %p start looping\n"
            ,__FILE__,__FUNCTION__,__LINE__, this);

    while (!quit_)
    {
        // 清空存储就绪sockfd对应的Channel的数组
        activeChannels_.clear();
        // 底层就是调用epoll_wait,并返回就绪的sockfd,一般阻塞在这里的等待新的链接或者读写事件
        pollReturnTime_ = poller_->Poll(KPollTimeMs, &activeChannels_);

        for (Channel *channel : activeChannels_)
        {
            // Poller监听的channel事件发生了，然后上报EventLoop，通知Channel处理事件
            // 就是调用发生事件的sockfd对应的回调函数(read/write/close)
            channel->handleEvent(pollReturnTime_);
        }
        /** 
         * 这里执行这个函数主要实在每次wakeup当前的loop
         * 时处理的，主要就是给subloop添加新的连接
         */
        doPendingFunctors();
    }

    LOG_INFO("%s:%s:%d  EventLoop %p stop looping\n"
                ,__FILE__,__FUNCTION__,__LINE__, this);

    looping_ = false;
}

// 这个函数其实主要就是调用mainloop给subloop注册的回调函数
// 处理的就是给subloop分配新的连接
// 或者是subloop调用回调关闭连接
void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;

    {
        std::unique_lock<std::mutex> lock(mutex_);
        // 使用局部的一个functors交换pendingFunctors，
        // 这样可以解放pendingFunctors，
        // 然后让等待pendingFunctors的其他线程获取到锁后继续使用pendingFunctors
        functors.swap(pendingFunctors_);
    }

    // 执行回调
    for (const Functor &functor : functors)
    {
        functor();
    }

    callingPendingFunctors_ = false;
}

/** 
 * 退出事件循环
 * 1、loop在自己线程调用quit
 */
void EventLoop::quit()
{
    quit_ = true;

    // 2、如果是在其他线程中(subloop)调用quit
    // 首先需要wakeup当前线程，然后再退出
    if (!isInLoopThread())
    {
        wakeup();
    }
}

// 调用回调
void EventLoop::runInLoop(Functor cb)
{
    if (isInLoopThread())
    { // 如果当前的loop就属于当前这个线程就直接调用回调函数
        cb();
    }
    else // 如果当前的loop就不属于当前线程，就唤醒loop所在线程
    {
        queueINLoop(cb);
    }
}

// 将回调放入队列中
void EventLoop::queueINLoop(Functor cb)
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(cb); // empace_back直接在底层构造一个 push_back是拷贝构造一个
    }
    
    // 唤醒相应需要执行上面cb回调的线程
    // || callingPendingFunctors_: 当前loop正在执行回调，但是此时又有新的回调加入，因此就需要再次唤醒loop，
    if (!isInLoopThread() || callingPendingFunctors_)
    {
        wakeup(); // 唤醒loop所在线程
    }
}

// 用来唤醒loop所在的线程,向wakeupFd_写一个数据，然后线程就会被唤醒 --> 对应的读就是handleRead
void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = ::write(wakeupFd_, &one, sizeof one);
    if (n != sizeof one)
    {
        LOG_ERROR("%s:%s:%d  EventLoop::wakeup writes %lu bytes instead of 8\n"
                ,__FILE__,__FUNCTION__,__LINE__, n);
    }
}

// wakeup使用 给每个EventLoop注册的回调，用于唤醒EventLoop使用
void EventLoop::handleRead() 
{
    uint64_t one = 1;
    ssize_t n = ::read(wakeupFd_, &one, sizeof one);
    if (n != sizeof one)
    {
        LOG_ERROR("%s:%s:%d  EventLoop::handleRead() error : %d \n"
                ,__FILE__,__FUNCTION__,__LINE__, errno);
    }
}

void EventLoop::updateChannel(Channel *channel)
{
    poller_->updateChannel(channel);
}
void EventLoop::removeChannel(Channel *channel)
{
    poller_->removeChannel(channel);
}
bool EventLoop::hasChannel(Channel *channel)
{
    return poller_->hasChannel(channel);
}
