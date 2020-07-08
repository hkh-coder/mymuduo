#include "Socket.h"
#include "InetAddress.h"
#include "Logger.h"

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <strings.h>
#include <netinet/tcp.h>

Socket::~Socket()
{
    ::close(sockfd_);
}
// 这个就是绑定listenfd和服务器的地址-> Liunx上的bind函数
void Socket::bindAddress(const InetAddress &localaddr)
{
    if(::bind(sockfd_,(sockaddr*)localaddr.getsockaddr(),sizeof(sockaddr_in)) != 0)
    {
        LOG_FATAL("%s:%s:%d   Socket::bind error %d \n"
                    ,__FILE__,__FUNCTION__,__LINE__,errno);
    }
}

// 调用Linux的系统API接口listen监听
void Socket::listen()
{
    if(::listen(sockfd_,SOMAXCONN) != 0)
    {
        LOG_FATAL("%s:%s:%d   Socket::listen error %d \n"
                ,__FILE__,__FUNCTION__,__LINE__,errno);
    }
}

// 调用Linux上的accept接口建立新的连接
int Socket::accept(InetAddress *peeraddr)
{
    sockaddr_in addr;
    socklen_t len = sizeof addr;
    bzero(&addr,sizeof(addr));
    // accept4系统可直接指定这个连接的fd是非阻塞的
    int connfd = ::accept4(sockfd_,(sockaddr*)&addr,&len,SOCK_NONBLOCK | SOCK_CLOEXEC);
    if(connfd >= 0)
    {
        peeraddr->setSockAddrInet(addr);
    }
    return connfd;
}

// 调用Linux上的shutdown接口关闭连接
void Socket::shutdownWrite()
{
    if(::shutdown(sockfd_,SHUT_WR) < 0)
    {
        LOG_ERROR("%s:%s:%d   Socket::shutdownWrite error %d \n"
                ,__FILE__,__FUNCTION__,__LINE__,errno);
    }
}

// 设置TCP连接选项
void Socket::setTcpNoDelay(bool on)
{
    int optval = on ? 1:0;
    ::setsockopt(sockfd_,IPPROTO_TCP,TCP_NODELAY,&optval,sizeof optval);
}

void Socket::setReuseAddr(bool on)
{
    int optval = on ? 1:0;
    ::setsockopt(sockfd_,SOL_SOCKET,SO_REUSEADDR,&optval,sizeof optval);
}

void Socket::setReusePort(bool on)
{
    int optval = on ? 1:0;
    ::setsockopt(sockfd_,SOL_SOCKET,SO_REUSEPORT,&optval,sizeof optval);
}
void Socket::setKeepAlive(bool on)
{
    int optval = on ? 1:0;
    ::setsockopt(sockfd_,SOL_SOCKET,SO_KEEPALIVE,&optval,sizeof optval);
}