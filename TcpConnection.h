#pragma once

#include "Buffer.h"
#include "Callbacks.h"
#include "InetAddress.h"
#include "Timestamp.h"
#include "noncopyable.h"

#include <atomic>
#include <memory>
#include <string>

// 前置声明
class Channel;
class EventLoop;
class Socket;

// TcpConnection和socket打包成Channel，扔给poller
// poller在监听到channel上有事件发生以后，就会去回调
// 这些callback都是channel从Tcpconnection拿过来的
// 用户给Tcpserver，Tcpserver再给Tcpconnection，Tcpconnection最终把这些东西设置给channel，channel就可以加入poller，poller监听到channel有相应事件发生，channel就会执行相应的回调

/**
 * TcpServer => Acceptor => 有一个新用户连接，通过accept函数拿到connfd
 * =》 TcpConnection 设置回调 =》 Channel =》 Poller =》 Channel的回调操作
 *
 */
class TcpConnection : noncopyable,
                      public std::enable_shared_from_this<TcpConnection> {
public:
    // sockfd是TcpServer给我的
    TcpConnection(EventLoop* loop,
                  const std::string& name,
                  int sockfd,
                  const InetAddress& localAddr,
                  const InetAddress& peerAddr);
    ~TcpConnection();

    EventLoop* getLoop() const { return loop_; }
    const std::string& name() const { return name_; }
    const InetAddress& localAddress() const { return localAddr_; }
    const InetAddress& peerAddress() const { return peerAddr_; }

    // 是否已连接
    bool connected() const { return state_ == kConnected; }

    // 发送数据
    void send(const std::string &buf);
    // 关闭连接，外部要调用的不能写到私有里面
    void shutdown();

    void setConnectionCallback(const ConnectionCallback& cb) {
        connectionCallback_ = cb;
    }

    void setMessageCallback(const MessageCallback& cb) {
        messageCallback_ = cb;
    }

    void setWriteCompleteCallback(const WriteCompleteCallback& cb) {
        writeCompleteCallback_ = cb;
    }

    void setHighWaterMarkCallback(const HighWaterMarkCallback& cb,
                                  size_t highWaterMark) {
        highWaterMarkCallback_ = cb;
        highWaterMark_ = highWaterMark;
    }

    void setCloseCallback(const CloseCallback& cb) { closeCallback_ = cb; }

    // 连接建立
    void connectEstablished();
    // 连接销毁
    void connectDestroyed();

private:
    enum StateE { kDisconnected, kConnecting, kConnected, kDisconnecting };
    void setState(StateE state) { state_ = state; }

    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();

    void sendInLoop(const void* message, size_t len);
    void shutdownInLoop();

    // 这里绝对不是baseLoop,因为TcpConnection都是在subLoop里面管理的
    EventLoop* loop_;
    const std::string name_;
    std::atomic_int state_;
    bool reading_;

    // 这里和Acceptor类似   Acceptor=》mainLoop    TcpConenction=》subLoop
    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;

    // 当前主机
    const InetAddress localAddr_;
    // 对端的
    const InetAddress peerAddr_;

    ConnectionCallback connectionCallback_;  // 有新连接时的回调
    MessageCallback messageCallback_;        // 有读写消息时的回调
    WriteCompleteCallback writeCompleteCallback_;  // 消息发送完成以后的回调
    HighWaterMarkCallback highWaterMarkCallback_;
    CloseCallback closeCallback_;
    size_t highWaterMark_;  // 指定多少算水位线

    Buffer inputBuffer_;   // 接收数据的缓冲区
    Buffer outputBuffer_;  // 发送数据的缓冲区
};