#pragma once
#include "noncopyable.h"
#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <unistd.h> // pid_t

/**
 * 封装thread 实现一个Thread类
 */
class Thread : noncpoyable
{

public:
    // 定义线程函数
    using ThreadFunc = std::function<void()>;

    explicit Thread(ThreadFunc, const std::string &name = std::string());

    ~Thread();
    // 启动线程
    void start();
    // 设置join
    void join();

    bool started() const { return started_; }
    pid_t tid() const { return tid_; }
    const std::string &name() const { return name_; }

    static int numCreated() { return numCreated_; }

private:

    void setDefultName(); 

    bool started_;                          // 标记线程启动
    bool joined_;                           // 标记线程join
    std::shared_ptr<std::thread> threadId_; // 线程对象
    pid_t tid_;                             // 线程id号
    ThreadFunc func_;                       // 线程函数
    std::string name_;                           // 线程名字

    static std::atomic_int numCreated_; // 线程个数
};
