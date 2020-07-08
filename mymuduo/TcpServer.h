#pragma once

#include "Acceptor.h"
#include "Buffer.h"
#include "Callbacks.h"
#include "EventLoop.h"
#include "EventLoopThreadPool.h"
#include "InetAddress.h"
#include "TcpConnection.h"
#include "Timestamp.h"
#include "noncopyable.h"

#include <atomic>
#include <functional>
#include <string>
#include <unordered_map>

class TcpServer : noncpoyable
{

public:
    using ThreadInitCallback = std::function<void(EventLoop *)>;

    // 设置Port的选项
    enum Option
    {
        kNoReusePort,
        KReusePort,
    };
    
    TcpServer(EventLoop *loop, const InetAddress &listenAddr, const std::string nameArg,
              Option option = kNoReusePort);
    ~TcpServer();

    // 返回string类型的Ip和Port
    const std::string &ipPort() const { return ipPort_; }
    // 返回服务器名字
    const std::string &name() const { return name_; }
    // 返回服务器的主loop(mainloop)
    EventLoop *getLoop() const { return loop_; }

    // 设置线程数量，(一个线程一个loop per thread one loop)
    void setThreadNum(int numThreads);
    // 设置初始化线程的线程函数
    void setThreadInitCallback(const ThreadInitCallback &cb) { threadInitCallback_ = std::move(cb); }
    // 获取EventLoop线程池对象
    std::shared_ptr<EventLoopThreadPool> threadPool() { return threadPool_; }

    // 开启服务器
    void start();

    // 设置处理新连接的回调函数
    void setConnectionCallback(const ConnectionCallback &cb) { connectionCallback_ = std::move(cb); }
    // 设置处理已连接的数据读写回调函数
    void setMessageCallback(const MessageCallback &cb) { messageCallback_ = std::move(cb); }
    // 设置写事件已完成后的回调函数
    void setWriteCompleteCallback(const WriteCompleteCallback &cb) { writeCompleteCallback_ = std::move(cb); }

private:
    // 新的连接
    void newConnection(int sockfd, const InetAddress &peerAddr);
    // 移除一个连接
    void removeConnection(const TcpConnectionPtr &conn);
    // 从loop中移除一个连接
    void removeConnectionInLoop(const TcpConnectionPtr &conn);

    // 保存所有连接的数据结构(无序的map(hashmap))
    using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;

    EventLoop *loop_; // the acceptor loop (main loop)
    const std::string ipPort_;
    const std::string name_;
    std::unique_ptr<Acceptor> acceptor_;              // mainloop对应的acceptor
    std::shared_ptr<EventLoopThreadPool> threadPool_; // EventLoop线程池对象

    // 回调函数
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    ThreadInitCallback threadInitCallback_;

    std::atomic_int started_; // 标记TcpServer启动监听

    int nextConnId_;            // 表示连接数
    ConnectionMap connections_; //保存连接map表
};
