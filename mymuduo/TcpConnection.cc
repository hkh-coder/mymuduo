#include "TcpConnection.h"
#include "Channel.h"
#include "EventLoop.h"
#include "Logger.h"
#include "Socket.h"

// 检测baseloop是否为空，如果为空程序直接退出
static EventLoop *checkNotNull(EventLoop *loop)
{
    if (loop == nullptr)
    {
        LOG_FATAL("%s:%s:%d   baseloop is nullptr", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

TcpConnection::TcpConnection(EventLoop *loop,
                             const std::string &nameArg,
                             int sockfd,
                             const InetAddress &localAddr,
                             const InetAddress &peerAddr)
    : loop_(checkNotNull(loop))
    , name_(nameArg)
    , state_(KConnecting)
    , reading_(true)
    , socket_(new Socket(sockfd))
    , channel_(new Channel(loop, sockfd))
    , localAddr_(localAddr)
    , peerAddr_(peerAddr)
    , highWaterMark_(64 * 1024 * 1024) // 64M
{
    channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
    channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));

    LOG_INFO("%s:%s:%d   TcpConnection::TcpConnection [%s] at %p fd=%d\n"
                , __FILE__, __FUNCTION__, __LINE__, name_.c_str(), this, sockfd);

    socket_->setKeepAlive(true);
}
TcpConnection::~TcpConnection()
{
    LOG_INFO("%s:%s:%d   TcpConnection::~TcpConnection [%s] at %p fd=%d state=%d"
            , __FILE__, __FUNCTION__, __LINE__, name_.c_str(), this, channel_->fd(), (int)state_);
}

// 可读事件的回调
void TcpConnection::handleRead(Timestamp receiveTime)
{
    int savedErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
    if (n > 0)
    {
        // shared_from_this 获取当前对象的
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    }
    else if(n == 0)
    {
        handleClose();
    }
    else
    {
        errno = savedErrno;
        LOG_ERROR("%s:%s:%d   TcpConnection::handleRead error"
                    , __FILE__, __FUNCTION__, __LINE__);
        handleError();
    }
}

// 可写事件的回调
void TcpConnection::handleWrite()
{
    if (channel_->isWriteing())
    { // 当前连接注册了可写事件
        int savedErrno = 0;
        ssize_t n = outputBuffer_.writeFd(channel_->fd(), &savedErrno);
        if (n > 0)
        {
            outputBuffer_.retrieve(n); // 数据读取完毕，调整readIndex_的位置
            if (outputBuffer_.readableBytes() == 0)
            {                               // 数据已经写完
                channel_->disableWriting(); // 将fd设置为不可写
                if (writeCompleteCallback_)
                { // 调用写完成后的回调函数
                    loop_->queueINLoop(std::bind(writeCompleteCallback_, shared_from_this()));
                }
                // 如果写入数据后正在关闭连接，服务器也会断开连接
                if (state_ == KDisconnecting)
                {
                    shutdownInLoop();
                }
            }
        }
        else
        {
            LOG_ERROR("%s:%s:%d   TcpConnection::handlerWrite error %d\n"
                    , __FILE__, __FUNCTION__, __LINE__, errno);
        }
    }
    else
    {
        LOG_INFO("%s:%s:%d   TcpConnection::handleWrite Connection fd = %d is down no more writing\n"
                , __FILE__, __FUNCTION__, __LINE__, channel_->fd());
    }
}

// 关闭连接的回调
void TcpConnection::handleClose()
{
    LOG_INFO("%s:%s:%d   TcpConnection::handleClose fd=%d state=%d \n"
            , __FILE__, __FUNCTION__, __LINE__, channel_->fd(), (int)state_);
    setState(KDisconnected); // 设置连接状态为已关闭
    channel_->disableAll();  // 取消对关注的所有事件

    TcpConnectionPtr connPtr(shared_from_this());
    if (connectionCallback_)
    {
        connectionCallback_(connPtr); // 执行关闭连接的回调
    }
    if (closeCallback_)
    {
        closeCallback_(connPtr); // 关闭连接的回调
    }
}

// 处理错误的回调
void TcpConnection::handleError()
{
    int optval = 0;
    socklen_t optlen = sizeof optval;
    int err = 0;
    if (::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
    {
        err = errno;
    }
    else
    {
        err = optval;
    }

    LOG_ERROR("%s:%s:%d   TcpConnection::handlError [%s] - SO_ERROR %d \n"
                , __FILE__, __FUNCTION__, __LINE__, name_.c_str(), err);
}

void TcpConnection::send(const std::string &buf)
{
    if (state_ == KConnected)
    { // 已连接状态才可以可以发送数据
        if (loop_->isInLoopThread())
        { // 当前loop所属当前thread
            sendInLoop(buf.c_str(), buf.size());
        }
    }
    else
    { // 当前loop不属于当前线程，需要注册回调，让loop所属线程调用
        loop_->runInLoop(std::bind(&TcpConnection::sendInLoop, this, buf.c_str(), buf.size()));
    }
}

void TcpConnection::sendInLoop(const void *data, size_t len)
{
    ssize_t nwrote = 0;
    size_t remaining = len; // 表示还未写的数据长度
    bool faultError = false;
    if (state_ == KDisconnected)
    { // 如果连接已经关闭
        LOG_INFO("%s:%s:%d   TcpConnection::sennInLoop disconnected give up writing"
                , __FILE__, __FUNCTION__, __LINE__);
        return;
    }

    if (!channel_->isWriteing() && outputBuffer_.readableBytes() == 0)
    { // fd不关注写事件 并且 outputBuffer_缓冲区中的可读的数据为0
        nwrote = ::write(channel_->fd(), data, len);
        if (nwrote >= 0)
        {
            remaining = len - nwrote;
            if (remaining == 0 && writeCompleteCallback_)
            { // 表示数据已经发生完成,注册写完成的回调函数
                loop_->queueINLoop(std::bind(writeCompleteCallback_, shared_from_this()));
            }
        }
        else // nwrote < 0 出错
        {
            nwrote = 0;
            if (errno != EWOULDBLOCK)
            {
                LOG_ERROR("%s:%s:%d   Tcpconnection::sendInLoop write errno\n"
                        , __FILE__, __FUNCTION__, __LINE__);
                if (errno == EPIPE || errno == ECONNRESET)
                { // 客户端对connfd进行重置
                    faultError = true;
                }
            }
        }
    }

    if (!faultError && remaining > 0)
    { // write没有出错，并且数据没有写完

        // outbuffer_缓冲区中可读的数据长度
        size_t oldLen = outputBuffer_.readableBytes();

        if (oldLen + remaining >= highWaterMark_ && oldLen < highWaterMark_ && highWaterMarkCallback_)
        {
            loop_->queueINLoop(std::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
        }
        // 将为写完的数据先写入到outbuffer_缓冲区中
        outputBuffer_.append((char *)data + nwrote, remaining);

        if (!channel_->isWriteing())
        { // 如果fd未关注写事件，让fd关注写事件
            channel_->enableWriting();
        }
    }
}

void TcpConnection::shutdown()
{
    if (state_ == KConnected)
    { // 连接状态 ： 已连接
        // 设置连接状态为正在断开连接
        setState(KDisconnecting);
        loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
    }
}
void TcpConnection::shutdownInLoop()
{
    if (!channel_->isWriteing())
    { // fd没有数据在发送了
        // 关闭写端，这个会触发EPOLLHUP然后调用TcpConnection::handleClose
        socket_->shutdownWrite();
    }
}

// 建立连接
void TcpConnection::connectEstablished()
{
    // 设置连接状态为已连接
    setState(KConnected);
    // channel_监听TcpConnection是否存在 tie就是将当前TcpConnection给Channel让Channel监听着
    // 因为channel中调用的回调都是来自TcpConnection的
    channel_->tie(shared_from_this());
    channel_->enableReading(); // fd关注写事件

    // 调用新连接的回调
    connectionCallback_(shared_from_this());
}

// 销毁连接
void TcpConnection::connectDestroyed()
{
    if (state_ == KConnected)
    { // 连接状态为已连接
        // 将连接状态设置为正在关闭连接
        setState(KDisconnecting);
        // 取消fd关注的所有事件，并从Poller中删除 -> epoll内核事件表中删除
        channel_->disableAll(); 

        // 调用关闭连接的回调函数->这个是用户传递的一个回调或者默认的
        connectionCallback_(shared_from_this());
    }
    // 将channel从EventLoop中删除
    // 并且将这个fd从epoll内核事件表中删除
    channel_->remove(); 
}