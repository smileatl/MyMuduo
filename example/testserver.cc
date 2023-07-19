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
        server_.setConnectionCallback(
            std::bind(&EchoServer::onConnection, this, std::placeholders::_1));

        server_.setMessageCallback(
            std::bind(&EchoServer::onMessage, this, std::placeholders::_1,
                      std::placeholders::_2, std::placeholders::_3));

        // 设置合适的loop线程数量  loopthread
        server_.setThreadNum(3);
    }

    void start() { server_.start(); }

private:
    // 连接建立或者断开的回调
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
    EventLoop loop;
    InetAddress addr(6000);
    // 创建了Acceptor对象 non-blocking listenfd create bind
    EchoServer server(&loop, addr, "EchoServer-01");
    // listen  loopthread  listenfd => acceptChannel => mainLoop =>
    server.start();

    loop.loop();  // 启动mainloop的底层Poller

    return 0;
}