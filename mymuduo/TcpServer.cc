#include "TcpServer.h"
#include <strings.h>

// 检测baseloop是否为空，如果为空程序直接退出
static EventLoop *checkNotNull(EventLoop *loop)
{
    if (loop == nullptr)
    {
        LOG_FATAL("%s:%s:%d  baseloop is nullptr", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

TcpServer::TcpServer(EventLoop *loop, const InetAddress &listenAddr, const std::string nameArg,
                     Option option)
    : loop_(checkNotNull(loop)) // 用户传递的主loop baseloop
    , ipPort_(listenAddr.toIpPort()) // 用户传递的listenfd的地址(服务器的ip和port)
    , name_(nameArg) // 用户传递的服务器名
    , acceptor_(new Acceptor(loop, listenAddr, option == KReusePort)) // 创建listenfd对应的Acceptor
    , threadPool_(new EventLoopThreadPool(loop, name_)) // 创建Eventloop线程池
    , connectionCallback_() // 新连接到来的回调
    , messageCallback_() // 已连接数据到来的回调
    , nextConnId_(1)
    , started_(0)
{
    // 给Acceptor设置的处理新的连接的回调函数TcpServer::newConnection
    acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection, this, std::placeholders::_1, std::placeholders::_2));
}

TcpServer::~TcpServer()
{
    LOG_INFO("%s:%s:%d   TcpServer::~TcpServer [%s] destructing\n", __FILE__, __FUNCTION__, __LINE__, name_.c_str());
    for (auto &item : connections_)
    {
        // 使用一个局部的TcpConnectionPtr接收
        TcpConnectionPtr conn(item.second);
        item.second.reset(); // 将item智能指针重置
        // 从conn所属的loop中将删除
        conn->getLoop()->runInLoop(
            std::bind(&TcpConnection::connectDestroyed, conn));
        // 局部的TcpConnectionPtr在出作用域后析构掉这个连接
    }
}

// 设置线程数量，(一个线程一个loop per thread one loop)
void TcpServer::setThreadNum(int numThreads)
{
    threadPool_->setThreadNum(numThreads);
}

// 开启服务器
void TcpServer::start()
{
    if (started_++ == 0) // ++操作是为了防止同一个TcpServer的start被重复调用
    {
        // 启动EventLoop线程池(创建用户设置的数量个线程)
        threadPool_->start(threadInitCallback_);
        // 调用runInloop->Acceeptor::listen->::listen开始监听
        loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_.get()));
    }
}

// 打包新的连接，并分发给subloop
void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr)
{
    // 选择一个subloop -> 这里采用轮询的方式进行选择
    EventLoop *ioloop = threadPool_->getNextLoop();

    // 封装这个新连接的名字
    char buf[64] = {0};
    snprintf(buf, sizeof buf, "-%s#%d", ipPort_.c_str(), nextConnId_);
    ++nextConnId_;
    std::string connName = name_ + buf;

    LOG_INFO("%s:%s:%d   TcpServer::newConnection [%s] - new connection [%s] from %s \n", __FILE__, __FUNCTION__, __LINE__,
             name_.c_str(), connName.c_str(), peerAddr.toIpPort().c_str());

    // 获取本地localAddr 也就是获取当前服务器的地址
    sockaddr_in localaddr;
    bzero(&localaddr, sizeof localaddr);
    socklen_t addrlen = sizeof localaddr;
    if (::getsockname(sockfd, (sockaddr *)&localaddr, &addrlen) < 0)
    {
        LOG_ERROR("%s:%s:%d   TcpServer::newConnection getsockname error\n", __FILE__, __FUNCTION__, __LINE__);
    }
    InetAddress localAddr(localaddr);

    // 创建一个新的连接，使用智能指针管理(TcpConnectionPtr)
    TcpConnectionPtr conn(new TcpConnection(ioloop, connName, sockfd, localAddr, peerAddr));
    // 将新的连接放入TcpServer保存连接的map中
    connections_[connName] = conn;

    // 设置这个连接的回调操作
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback(std::bind(&TcpServer::removeConnection, this, std::placeholders::_1));

    ioloop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
}

// 移除一个连接
void TcpServer::removeConnection(const TcpConnectionPtr &conn)
{
    loop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

// 从loop中移除一个连接
void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn)
{
    LOG_INFO("%s:%s:%d   TcpServer::removeConnectionInLoop [%s] -connection [%s]\n", __FILE__, __FUNCTION__, __LINE__, name_.c_str(), conn->name().c_str());

    // 从TcpServer的map中删除
    connections_.erase(conn->name());
    // 获取当前连接所属的loop
    EventLoop *ioLoop = conn->getLoop();
    // 注册回调函数TcpConnection::connectDestroyed 将conn连接删除掉
    ioLoop->queueINLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}
