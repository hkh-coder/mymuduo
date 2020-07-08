#pragma once
#include "noncopyable.h"
#include "Timestamp.h"

#include <unordered_map>
#include <vector>

class EventLoop;
class Channel;
/**
 * Poller一个poll和epoll的基类
 * muduo中多路事件分发器的核心IO复用模块
 */
class Poller : noncpoyable
{

public:
    using ChannelList = std::vector<Channel *>;
    Poller(EventLoop *loop):ownerLoop_(loop){}
    virtual ~Poller() = default;
    
    // 给所有IO复用保留同一的接口,纯虚函数
    virtual Timestamp Poll(int timeoutMs,ChannelList *activeChannels) = 0;
    
    // 更新poller中的Channel
    virtual void updateChannel(Channel*channel) = 0;

    // 删除Poller中注册的Channel
    virtual void removeChannel(Channel*channel) = 0;

    // 判断参数Channel时候在当前Poller中
    bool hasChannel(Channel*channel)const;

    // EventLoop事件循环通过该接口获取默认的IO复用的具体实现(poll / epoll)
    // 这个接口在派生类中实现的。因为只有当派生出poll或者epoll才知道如何实现这个函数
    static Poller* newDefaultPoller(EventLoop *loop);
protected:
    // 使用hashTable保存Poller监听的Channel
    // hashtable的key->sockfd,value->sockfd对应的Channel
    using ChannelMap = std::unordered_map<int, Channel *>;
    ChannelMap channels_;

private:
    // 保存Poller所属的EventLoop
    EventLoop *ownerLoop_;
};
