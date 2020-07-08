#include "Channel.h"
#include "EventLoop.h"
#include <sys/epoll.h>

// 用三个变量表示事件
const int Channel::KNoneEvent = 0;
const int Channel::KReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::KWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop *loop, int fd)
    : loop_(loop),
      fd_(fd),
      events_(0),
      revents_(0),
      index_(-1),
      tied_(false)
{
}
Channel::~Channel()
{
}

// 防止当Channel被手动remove掉，Channel还在执行回调操作
// 通过weak_ptr来监听Channel是否存在。
void Channel::tie(const std::shared_ptr<void> &obj)
{
    tie_ = obj;
    tied_ = true;
}

/**
 *  update更新sockfd关注的事件
 * 通过Channel所属的EventLoop，调用poller的相应方法，更新fd关注的事件
*/
void Channel::update()
{
    // loop->updateChannel -> poller->updateChannel->epoll_ctl
    loop_->updateChannel(this);
}

// 从Channel所属的EventLoop中删除当前的Channel
void Channel::remove()
{
    // loop->removeChannel -> poller->removeChannel->epoll_ctl
    loop_->removeChannel(this);
}

// fd得到poller通知以后，处理事件的函数模块
void Channel::handleEvent(Timestamp receiveTime)
{
    if (tied_)
    {
        std::shared_ptr<void> guard = tie_.lock();
        if (guard)
        {
            handlerEventWithGuard(receiveTime);
        }
    }
    else
    {
        handlerEventWithGuard(receiveTime);
    }
}

// 根据相应的事件调用相应的回调函数
void Channel::handlerEventWithGuard(Timestamp receiveTime)
{
    LOG_INFO("%s:%s:%d Channel handlerEvent revents : %d\n"
            ,__FILE__,__FUNCTION__,__LINE__,revents_);
    
    // 关闭连接事件
    if((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))
    {
        if(closeCallback_)
        {
            closeCallback_();
        }
    } 

    // 出错事件
    if(revents_ & EPOLLERR)
    {
        if(errorCallback_)
        {
            errorCallback_();
        }
    }

    // 可读事件
    if(revents_ & (EPOLLIN | EPOLLPRI))
    {
        if(readCallback_)
        {
            readCallback_(receiveTime);
        }
    }

    // 可写事件
    if(revents_ & EPOLLOUT)
    {
        if(writeCallback_)
        {
            writeCallback_();
        }
    }
}