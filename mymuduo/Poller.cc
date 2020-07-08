#include "Poller.h"
#include "Channel.h"

// 判断参数Channel时候在当前Poller中
bool Poller::hasChannel(Channel *channel) const
{
    auto it = channels_.find(channel->fd());
    return it != channels_.end() && it->second == channel;
}