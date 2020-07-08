#pragma once
#include "noncopyable.h"
#include "Socket.h"
#include "Channel.h"

#include <functional>


class EventLoop;
class InetAddress;

/**
 * Acceptor类主要是封装的是listenfd(服务器fd)
 * 主要作用就是封装新的连接，并且协助TcpServer将新的连接分发给subloop
 */

class Acceptor : noncpoyable
{

public:
    using NewConnectionCallback = std::function<void(int sockfd,const InetAddress&)>;

    Acceptor(EventLoop *loop,const InetAddress &listenAddr,bool reuseport);
    ~Acceptor();

    void setNewConnectionCallback(const NewConnectionCallback&cb){newConnectionCallback_ = std::move(cb);}
    bool listenning()const {return listenning_;}
    void listen();
private:

    void handleRead();

    EventLoop *loop_; // 这个loop就是mainloop，
    Socket acceptSocket_; // 这个是服务器对应的listenfd
    Channel acceptChannel_; // 这个是服务器对应的channel
    NewConnectionCallback newConnectionCallback_;
    bool listenning_;
};