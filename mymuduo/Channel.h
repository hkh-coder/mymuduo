#pragma once
#include <functional>
#include <memory>

#include "Timestamp.h"
#include "noncopyable.h"
#include "Logger.h"

class EventLoop; // 类型声明

/**
 * 理清楚EventLoop,Channel,Poller之间的关系，=》 对应Reactor模型上的Demultiplex事件多路分发器
 * Channel理解为通道，封装了sockfd和其感兴趣的事件
 * 还绑定了poller返回的具体就绪事件revents_
 */

class Channel : noncpoyable
{

public:
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(Timestamp)>;

    Channel(EventLoop *loop, int fd);
    ~Channel();

    // fd得到poller通知以后，处理事件的
    void handleEvent(Timestamp receiveTime);

    // 设置回调函数对象
    void setReadCallback(ReadEventCallback cb)
    {
        // 转移成右值
        readCallback_ = std::move(cb);
    }
    void setWriteCallback(EventCallback cb)
    {
        writeCallback_ = std::move(cb);
    }
    void setCloseCallback(EventCallback cb)
    {
        closeCallback_ = std::move(cb);
    }
    void setErrorCallback(EventCallback cb)
    {
        errorCallback_ = std::move(cb);
    }

    // 防止当Channel被手动remove掉，Channel还在执行回调操作
    // 通过weak_ptr来监听Channel是否存在。
    void tie(const std::shared_ptr<void> &obj);

    // 返回sockfd文件描述符
    int fd() const { return fd_; }
    // 返回感兴趣的事件
    int events() const { return events_; }
    // 设置感兴趣的事件
    void set_revents(int revt) { revents_ = revt; }
    // 是否有感兴趣的事件
    bool isNoneEvent() const { return events_ == KNoneEvent; }
    // 是否关注写事件
    bool isWriteing() const { return events_ & KWriteEvent; }
    // 是否关注读时间
    bool isReading() const { return events_ & KReadEvent; }

    // 给sockfd添加读事件监听
    void enableReading()
    {
        events_ |= KReadEvent;
        update();
    }
    // 取消sockfd的读事件监听
    void disableReading()
    {
        events_ &= ~KReadEvent;
        update();
    }
    // 给sockfd添加写事件监听
    void enableWriting()
    {
        events_ |= KWriteEvent;
        update();
    }
    // 取消sockfd的写事件监听
    void disableWriting()
    {
        events_ &= ~KWriteEvent;
        update();
    }
    // 取消所有的事件监听
    void disableAll()
    {
        events_ = KNoneEvent;
        update();
    }

    //
    int index() const { return index_; }
    void set_index(int idx) { index_ = idx; }

    // 返回当前Channel所属的EventLoop
    EventLoop *ownerLoop() { return loop_; }
    // 从Channel所属的EventLoop中删除当前的Channel
    void remove();

private:
    // update负责在poller中更新sockfd关注的事件
    void update();

    // 
    void handlerEventWithGuard(Timestamp receiveTime);

    // 用三个变量表示fd关心的事件
    static const int KNoneEvent; // 没有关心的事件
    static const int KReadEvent; // 关心读事件
    static const int KWriteEvent;// 关心写事件

    EventLoop *loop_; // 事件循环
    const int fd_;    // fd Poller监听的对象
    int events_;      // 注册fd感兴趣的事件
    int revents_;     // fd发生的事件
    int index_;       // poller使用的

    // 其实tie_指向的是当前channel对应的TcpConnection对象
    std::weak_ptr<void> tie_; 
    bool tied_;

    // 因为Channel通道里边能够获知fd最终发生的事件revents_
    // 所以Channel负责调用具体的事件驱动
    // 这四个函数都是由channel对应的TcpConnection提供的
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
};
