#include "Acceptor.h"
#include "Logger.h"
#include "EventLoop.h"
#include "InetAddress.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>

// 调用socket创建listenfd
static int createNonblocking()
{
    // 创建一个TCPlistenfd，且是非阻塞的
    int sockfd = ::socket(AF_INET,SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,0);
    if(sockfd < 0)
    {
        LOG_FATAL("%s:%s:%d Acceptor::createNonblocking error %d\n",__FILE__,__FUNCTION__,__LINE__,errno);
    }
    return sockfd;
}

Acceptor::Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport)
    : loop_(loop)
    , acceptSocket_(createNonblocking())  // 创建listenfd
    , acceptChannel_(loop,acceptSocket_.fd()) // listenfd封装成Channel
    , listenning_(false)
{
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.setReusePort(reuseport);
    acceptSocket_.bindAddress(listenAddr); // 给listenfd绑定地址
    // 给channel注册的处理读事件的回调，其实就是listenfd处理新连接的回调
    acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead,this));
}
Acceptor::~Acceptor()
{
    // 取消所有关注的事件
    acceptChannel_.disableAll();
    // 从所属的poller中移除
    acceptChannel_.remove();
}

// 启动listenfd监听可读事件
void Acceptor::listen()
{
    listenning_ = true;
    // listen
    acceptSocket_.listen();
    // 设置监听可读事件，其实就是开始监听新的连接的到来
    acceptChannel_.enableReading();
}

// 回调函数
void Acceptor::handleRead()
{
    InetAddress peerAddr;
    // 建立新的连接->其调用的就是linux上的accept函数
    int confd = acceptSocket_.accept(&peerAddr);
    if(confd >=0)
    {
        // 这个回调函数是由TcpServer提供，用来处理新的连接分发给subloop使用
        if(newConnectionCallback_)
        {
            // 对应TcpServer::newConnection
            newConnectionCallback_(confd,peerAddr);
        }
        else
        {
            // 如果没有处理新的连接就将这个连接关闭
            ::close(confd);
        }
    }
    else
    {
        LOG_ERROR("in Acceptor::handleRead error %d\n",errno);
        if(errno == EMFILE)
        {   // 这个错误表示服务器的文件描述符达到上限，需要更高的并发
            LOG_ERROR("%s:%s:%d Acceptor::handleRead sockfd reached error %d\n"
                        ,__FILE__,__FUNCTION__,__LINE__,errno);
        }
    }
    
}