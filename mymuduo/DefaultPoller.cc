#include "Poller.h"
#include "EPollPoller.h"
#include <stdlib.h>

Poller* Poller::newDefaultPoller(EventLoop *loop)
{
    /**
     * 当环境变量中定义了MUDUO_USE_POLL时才使用poll IO复用
     * 否则默认使用epoll IO复用
     */
    if(::getenv("MUDUO_USE_POLL"))
    {
        return nullptr; // 返回poll的实例
    }
    else
    {
        return new EPollPoller(loop); // 返回epoll的实例
    }
    
}