#include <mymuduo/Logger.h>
#include <mymuduo/TcpServer.h>
#include <functional>
#include <string>

class EchoServer {
public:
    EchoServer(EventLoop* loop,
               const InetAddress& addr,
               const std::string& name)
        : server_(loop, addr, name), loop_(loop) {
        // 注册回调函数
        // 将用户定义的连接事件处理函数注册进TcpServer中，TcpServer发生连接事件时会执行onConnection函数。
        server_.setConnectionCallback(
            std::bind(&EchoServer::onConnection, this, std::placeholders::_1));

        //将用户定义的可读事件处理函数注册进TcpServer中，TcpServer发生可读事件时会执行onMessage函数。
        server_.setMessageCallback(
            std::bind(&EchoServer::onMessage, this, std::placeholders::_1,
                      std::placeholders::_2, std::placeholders::_3));

        // 设置合适的loop线程数量  loopthread
        server_.setThreadNum(3);
    }

    void start() { server_.start(); }

private:
    // 连接建立或者断开的回调
    // 用户定义的连接事件处理函数：当服务端接收到新连接建立请求，则打印Connection UP，如果是关闭连接请求，则打印Connection Down
    void onConnection(const TcpConnectionPtr& conn) {
        if (conn->connected()) {
            LOG_INFO("Connection UP : %s",
                     conn->peerAddress().toIpPort().c_str());
        } else {
            LOG_INFO("Connection DOWN : %s",
                     conn->peerAddress().toIpPort().c_str());
        }
    }

    // 可读写事件回调
    void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp time) {
        // 服务器获取客户端发送的数据
        std::string msg = buf->retrieveAllAsString();
        // 服务器将数据原封不动的发送给客户端
        conn->send(msg);
        // 发送完关闭，删除掉连接
        conn->shutdown();  // 写端   EPOLLHUP =》 closeCallback_
    }

    EventLoop* loop_;
    TcpServer server_;
};

int main() {
    //这个EventLoop就是main EventLoop，即负责循环事件监听处理新用户连接事件的事件循环器。
    EventLoop loop;

    //InetAddress其实是对socket编程中的sockaddr_in进行封装，使其变为更友好简单的接口而已。
    InetAddress addr(6000);

    // 创建了Acceptor对象 non-blocking listenfd create bind
    EchoServer server(&loop, addr, "EchoServer-01");

    // listen  loopthread  listenfd => acceptChannel => mainLoop =>
    server.start();

    loop.loop();  // 启动mainloop的底层Poller

    return 0;
}