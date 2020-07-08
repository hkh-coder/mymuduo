#include "EPollPoller.h"
#include "Channel.h"
#include "Logger.h"

#include <string>
#include <strings.h>
#include <sys/epoll.h>
#include <unistd.h>

const int KNew = -1;    // Channel未添加到Poller中
const int KAdded = 1;   // Channel已添加到Poller中
const int KDeleted = 2; // Channel从Poller删除

EPollPoller::EPollPoller(EventLoop *Loop)
    : Poller(Loop),
      epollfd_(::epoll_create1(EPOLL_CLOEXEC)), // 创建一个内核事件表
      events_(KInitEventListSize)               // 初始化epoll数组
{
    if (epollfd_ < 0)
    {
        LOG_FATAL("%s:%s:%d epoll_create error : %d\n", __FILE__, __FUNCTION__, __LINE__, errno);
    }
}
EPollPoller::~EPollPoller()
{
    ::close(epollfd_);
}

// poll -> epoll_wait
Timestamp EPollPoller::Poll(int timeoutMs, ChannelList *activeChannels)
{
    LOG_INFO("%s:%s:%d  EPollPoller::Poll  fd total count %lu\n"
            , __FILE__, __FUNCTION__, __LINE__, channels_.size());
    // 启动epoll_wait循环，
    int numEvents = ::epoll_wait(epollfd_,
                                 &*events_.begin(), // 获取vector底层数组的指针
                                 static_cast<int>(events_.size()),
                                 timeoutMs);
   
    int savedErrno = errno; // 保存一下全局的errno
    // 获取一下当前时间
    Timestamp now(Timestamp::now());

    if (numEvents > 0)
    { // 表示有监听的sockfd上有事件发生
        LOG_INFO("%s:%s:%d    %d events happend\n", __FILE__, __FUNCTION__, __LINE__, numEvents);

        fillActionChannels(numEvents, activeChannels);

        // 如果当前返回发生事件的sockfd等于数组大小了就将数组2倍扩容
        if (numEvents == events_.size())
        {
            events_.resize(events_.size() * 2);
        }
    }
    else if (numEvents == 0)
    { // 监听的sockfd上没有事件发生
        LOG_INFO("%s:%s:%d   nothing events happend timeout!\n"
                    , __FILE__, __FUNCTION__, __LINE__);
    }
    else
    { // 发生错误 error happens
        if (savedErrno != EINTR)
        {
            errno = savedErrno;
            LOG_ERROR("%s:%s:%d   EPollPoller::poll error : %d \n", __FILE__, __FUNCTION__, __LINE__, errno);
        }
    }
    return now;
}

void EPollPoller::updateChannel(Channel *channel)
{
    // 获取当前Channel在Poller中的状态
    // channel的index的值对应于channel在poller中的状态
    const int index = channel->index();

    LOG_INFO("%s:%s:%d  fd = %d events = %d index = %d\n",
             __FILE__, __FUNCTION__, __LINE__,
             channel->fd(), channel->events(), index);

    if (index == KNew || index == KDeleted)
    {
        // a new one add with EPOLL_CTL_ADD
        if (index == KNew)
        { // index == KNew表示当前Channel不在Poller中
            int fd = channel->fd(); // 获取当前Channel的fd
            // 将当前Channel添加到Poller中
            channels_[fd] == channel;
        }
        // 将当前channel设置为KAdded表示已添加到channel
        channel->set_index(KAdded);

        // 更新Channel对应的fd中关注的事件 -> 在epoll内核事件表
        update(EPOLL_CTL_ADD, channel);
    }
    else
    { // update existing one with EPOLL_CTL_MOD/DEL
        int fd = channel->fd();
        if (channel->isNoneEvent())
        {                                   // 表示当前sockfd自己没有感兴趣的事件了，可以从当前Poller中删除了
            update(EPOLL_CTL_DEL, channel); // 从内核事件表中删除
            channel->set_index(KDeleted);   // 将channel标记为已从当前poller中删除
        }
        else
        {
            // 在内核事件表中更新channel对应fd关注的事件
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

void EPollPoller::removeChannel(Channel *channel)
{
    // 从Poller中的map中删除fd以及对应的Channel
    int fd = channel->fd();
    channels_.erase(fd);
    LOG_INFO("%s:%s:%d  fd = %d\n", __FILE__, __FUNCTION__, __LINE__, fd);

    // 获取index查看当前channel是KAdded/KDeleted
    int index = channel->index();

    if (index == KAdded)
    { // 如果是KAdded状态，还需要再epoll内核事件表中删除fd
        update(EPOLL_CTL_DEL, channel);
    }

    // 在讲channel的index设置为KNew -> 未在Poller中注册状态。
    channel->set_index(KNew);
}

// 填写活跃的连接 -> 填写的channel主要用于Eventloop然后调用channel上的回调函数
void EPollPoller::fillActionChannels(int numEvents, ChannelList *activeChannels) const
{
    for (int i = 0; i < numEvents; ++i)
    {
        Channel *channel = static_cast<Channel *>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel);
    }
}

// 更新channel通道,调用epoll_ctl
void EPollPoller::update(int operation, Channel *channel)
{
    // 创建epoll_event并初始化为0
    epoll_event event;
    bzero(&event, sizeof event);
    int fd = channel->fd();

    event.events = channel->events();
    event.data.fd = fd;
    event.data.ptr = channel;

    // 调用epoll_ctl
    if (::epoll_ctl(epollfd_, operation, fd, &event) < 0)
    {
        if (operation == EPOLL_CTL_DEL)
        { // 删除失败
            LOG_ERROR("%s:%s:%d  epoll_ctl del error : %d\n", __FILE__, __FUNCTION__, __LINE__, errno);
        }
        else
        { // 添加或者修改失败
            LOG_FATAL("%s:%s:%d  epoll_ctl mod/add error : %d\n", __FILE__, __FUNCTION__, __LINE__, errno);
        }
    }
}
