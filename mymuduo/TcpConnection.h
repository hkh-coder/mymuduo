#pragma once
#include "Buffer.h"
#include "Callbacks.h"
#include "InetAddress.h"
#include "noncopyable.h"
#include "Timestamp.h"

#include <atomic>
#include <memory>
#include <string>

class Channel;
class EventLoop;
class Socket;

class TcpConnection : noncpoyable, public std::enable_shared_from_this<TcpConnection>
{

public:
    TcpConnection(EventLoop *loop,
                 const std::string &nameArg, 
                 int sockfd, 
                 const InetAddress &localAddr, 
                 const InetAddress &peerAddr);
    ~TcpConnection();

    EventLoop *getLoop() const { return loop_; }
    const std::string &name() const { return name_; }
    const InetAddress &localAddress() const { return localAddr_; }
    const InetAddress &peerAddress() const { return peerAddr_; }

    bool connected() const { return state_ == KConnected; }
    bool disconnected() const { return state_ == KDisconnected; }

    void send(const std::string &buf);
    void shutdown();

    // 设置回调
    void setConnectionCallback(const ConnectionCallback &cb) { connectionCallback_ = std::move(cb); }
    void setMessageCallback(const MessageCallback &cb) { messageCallback_ = std::move(cb); }
    void setWriteCompleteCallback(const WriteCompleteCallback &cb) { writeCompleteCallback_ = std::move(cb); }
    void setHighWaterMarkCallback(const HighWaterMarkCallback &cb) { highWaterMarkCallback_ = std::move(cb); }
    void setCloseCallback(const CloseCallback &cb) { closeCallback_ = std::move(cb); }

    Buffer *inputBuffer() { return &inputBuffer_; }
    Buffer *outputBuffer() { return &outputBuffer_; }

    // 建立连接
    void connectEstablished();
    // 销毁连接
    void connectDestroyed();

private:
    enum StateE
    {
        KDisconnected, // 连接已断开
        KConnecting,   // 正在连接
        KConnected,    // 已连接
        KDisconnecting // 正在断开连接
    };

    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();
    
    void sendInLoop(const void*message,size_t len);
    void shutdownInLoop();

    void setState(StateE s){ state_ = s;}

    EventLoop *loop_;
    const std::string name_;
    std::atomic_int state_;
    bool reading_;

    std::unique_ptr<Socket> socket_;    
    std::unique_ptr<Channel> channel_;
    const InetAddress localAddr_;
    const InetAddress peerAddr_;

    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    CloseCallback closeCallback_;
    HighWaterMarkCallback highWaterMarkCallback_;
    size_t highWaterMark_;

    Buffer inputBuffer_;   // 接收数据的缓冲区
    Buffer outputBuffer_;  // 发送数据的缓冲区
};
