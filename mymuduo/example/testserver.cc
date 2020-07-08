#include <mymuduo/TcpServer.h>
#include <mymuduo/Logger.h>
#include <string>
/**
 * 使用mymuduo写一个简单的回响服务器
 */

class EchoServer
{
public:
    EchoServer(EventLoop *loop, InetAddress &addr, std::string name)
        : mloop(loop), mser(loop, addr, name)
    {
        // 设置回调函数
        mser.setConnectionCallback(std::bind(&EchoServer::onConnectionCallback,this,std::placeholders::_1));

        mser.setMessageCallback(std::bind(&EchoServer::onMessageCallback,this,
                    std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));
        // 设置线程数量
        mser.setThreadNum(3);
    }
    void start()
    {
        mser.start(); // 启动监听listen
    }
private:
    void onConnectionCallback(const TcpConnectionPtr& conn)
    {
        if(conn->connected())
        {
            LOG_INFO("new client connected   localAddr:%s peerAddr:%s\n",
                    conn->localAddress().toIpPort().c_str(),conn->peerAddress().toIpPort().c_str());
        }
        else
        {
            LOG_INFO("one client disconnected  localAddr:%s peerAddr:%s\n",
                    conn->localAddress().toIpPort().c_str(),conn->peerAddress().toIpPort().c_str());
        }
    }

    void onMessageCallback(const TcpConnectionPtr &conn,Buffer *buf,Timestamp time)
    {
        // 服务器获取客户端发送的数据
        std::string data = buf->retrieveAllAsString();
        LOG_INFO("EchoServer::onMessageCallback from client recv data [%s]\n",data.c_str());
        // 服务器在将数据原封不动的发送给客户端
        conn->send(data);
        LOG_INFO("EchoServer::onMessageCallback recv data to client [%s]\n",data.c_str());
        // 在服务器回响完成后直接断开连接
        conn->shutdown();
    }
    EventLoop *mloop;
    TcpServer mser;
};

int main()
{
    EventLoop loop;
    InetAddress localAddr(6000);
    EchoServer ser(&loop,localAddr,"EchoServer");
    ser.start();
    loop.loop();
    return 0;
}
