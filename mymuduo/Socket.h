#pragma once
#include "noncopyable.h"

class InetAddress;

class Socket : noncpoyable
{

public:
    explicit Socket(int sockfd)
        : sockfd_(sockfd){}
    ~Socket();

    int fd()const{return sockfd_;}

    void bindAddress(const InetAddress& localaddr);
    void listen();
    int accept(InetAddress*peeraddr);

    // 关闭写端
    void shutdownWrite();

    // 设置TCP连接选项
    void setTcpNoDelay(bool on);
    void setReuseAddr(bool on);
    void setReusePort(bool on);
    void setKeepAlive(bool on);
private:
    const int sockfd_;
};
