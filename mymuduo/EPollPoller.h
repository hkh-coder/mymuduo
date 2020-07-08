#pragma once
#include "Poller.h"
#include "Timestamp.h"

#include <sys/epoll.h>
#include <vector>
/**
 * 派生类EPollPoller->封装epoll
 * epoll_create
 * epoll_ctl    add/mod/del
 * epoll_wait
 */
class EventLoop;
class Channel;

class EPollPoller : public Poller
{

public:
    EPollPoller(EventLoop *Loop);
    ~EPollPoller();

    // override 编译器强制方法覆盖
    Timestamp Poll(int timeoutMs, ChannelList *activeChannels) override;

    void updateChannel(Channel *channel) override;

    void removeChannel(Channel *channel) override;

private:
    // 初始化EventList大小
    static const int KInitEventListSize = 16;
    
    // 填写活跃的连接
    void fillActionChannels(int numEvents,ChannelList *activeChannels)const;
    // 更新channel通道
    void update(int operation,Channel*channel);

    //
    using EventList = std::vector<epoll_event>;

    // epollfd内核事件表句柄
    int epollfd_; 

    // 保存已就绪事件sockfd的数组
    EventList events_;
};
