#pragma once
#include <netinet/in.h> // sockaddr_in
#include <string>

// 封装socket的Ip&port
class InetAddress
{
public:
    // 传递端口号和IP地址的构造函数
    explicit InetAddress(uint16_t port = 0, std::string ip = "127.0.0.1");

    // 传递一个sockaddr地址的构造函数
    explicit InetAddress(const sockaddr_in &addr) : addr_(addr) {}

    // 获取IP地址
    std::string toIp() const;

    // 获取端口号
    uint16_t toPort() const;

    // 获取IP地址和端口号
    std::string toIpPort() const;

    // 获取sockaddr对象
    const sockaddr_in *getsockaddr()const { return &addr_; }
    // 这是地址
    void setSockAddrInet(const sockaddr_in &addr) { addr_ = addr; }

private:
    sockaddr_in addr_;
};
