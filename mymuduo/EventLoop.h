#pragma once
#include "CurrentThread.h"
#include "Timestamp.h"
#include "noncopyable.h"

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <unistd.h>
#include <vector>

class Channel;
class Poller;

class EventLoop : noncpoyable
{
public:
    // 回调函数类型
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    // 启动EventLoop
    void loop();
    // 退出EventLoop
    void quit();

    // 返回poller返回就绪sockfd的时间
    Timestamp pollReturnTime() const { return pollReturnTime_; }

    // 调用回调
    void runInLoop(Functor cb);
    // 将回调放入队列中
    void queueINLoop(Functor cb);

    void wakeup();
    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);
    bool hasChannel(Channel *channel);

    // 判断时候在当前进程中
    bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }

private:
    void handleRead(); // wakeup使用
    void doPendingFunctors();

    using ChannelList = std::vector<Channel *>;

    // CAS类型
    std::atomic_bool looping_;
    std::atomic_bool quit_;

    const pid_t threadId_; // 保存当前EventLoop所在线程的tid

    // 记录Poller返回就绪sockfd的事件
    Timestamp pollReturnTime_;

    // 当前的EventLoop的Poller
    std::unique_ptr<Poller> poller_;

    // mainEventLoop轮询唤醒subEventLoop并将新的连接加入到唤醒的subEventLoop
    // 用户线程之间通信的一个专有eventfd
    int wakeupFd_;
    // 封装线程通信的eventfd对应的Channel
    std::unique_ptr<Channel> wakeupChannel_;

    // EventLoop用户保存Poller返回就绪的sockfd对应的channel对象的
    ChannelList activeChannels_;

    // 这个主要控制当前loop是否正在执行回调
    std::atomic_bool callingPendingFunctors_;
    // 保存当前loop需要执行的回调函数
    std::vector<Functor> pendingFunctors_;
    // 确保回调函数容器线程安全的锁
    std::mutex mutex_;
};
