#pragma once

#include <algorithm>
#include <string>
#include <vector>

/// A buffer class modeled after org.jboss.netty.buffer.ChannelBuffer
///
/// @code
/// +-------------------+------------------+------------------+
/// | prependable bytes |  readable bytes  |  writable bytes  |
/// |                   |     (CONTENT)    |                  |
/// +-------------------+------------------+------------------+
/// |                   |                  |                  |
/// 0      <=      readerIndex   <=   writerIndex    <=     size
/// @endcode

// 网络库底层的缓冲器类型定义
class Buffer {
public:
    // 记录数据包长度的大小
    static const size_t kCheapPrepend = 8;
    // 数据包后面缓冲区的大小
    static const size_t kInitialSize = 1024;

    // 同样不允许默认生成对象
    // readerIndex_/writerIndex_没有数据先指向一个地方
    explicit Buffer(size_t initialSize = kInitialSize)
        : buffer_(kCheapPrepend + initialSize),
          readerIndex_(kCheapPrepend),
          writerIndex_(kCheapPrepend) {}

    size_t readableBytes() const { return writerIndex_ - readerIndex_; }

    size_t writableBytes() const { return buffer_.size() - writerIndex_; }

    // 0-readerIndex_
    size_t prependableBytes() const { return readerIndex_; }

    // 返回缓冲区中可读数据的起始地址
    const char* peek() const { return begin() + readerIndex_; }

    // 把数据从buffer转成C++的string类型
    // onMessage string <- Buffer
    void retrieve(size_t len) {
        if (len < readableBytes()) {
            readerIndex_ +=
                len;  // 应用只读取了刻度缓冲区数据的一部分，就是len，还剩下readerIndex_
                      // += len -> writerIndex_有数据没读
        } else {
            // len==readableBytes
            retrieveAll();
        }
    }

    void retrieveAll() {
        // 没的读了，可读的已经读完了，把writerIndex拉到readIndex相当于复位了
        readerIndex_ = writerIndex_ = kCheapPrepend;
    }

    // 把onMessage函数上报的Buffer数据，转成string类型的数据返回
    std::string retrieveAllAsString() {
        return retrieveAsString(readableBytes());  // 应用可读取数据的长度
    }

    std::string retrieveAsString(size_t len) {
        // 构造了一个result
        std::string result(peek(), len);  // 应用可读取数据的长度

        retrieve(
            len);  // 上面一句把缓冲区中可读的数据，已经读取出来，这里肯定要对缓冲区进行复位操作
        return result;
    }

    // buffer_.size() - writerIndex_    len
    void ensureWriteableBytes(size_t len) {
        if (writableBytes() < len) {
            makeSpace(len);  // 扩容函数
        }
    }

    // 复制整串字符串
    void append(const std::string& str) { append(str.c_str(), str.size()); }

    // 把[data, data+len]内存上的数据，添加到writable缓冲区当中
    void append(const char* data, size_t len) {
        ensureWriteableBytes(len);
        // 把要添加的数据拷贝到可写的缓冲区里面
        std::copy(data, data + len, beginWrite());
        writerIndex_ += len;
    }

    const char* findCRLF() const {
        // 使用 std::search() 函数在缓冲区的当前读位置（peek()
        // 返回的值）和写位置（beginWrite() 返回的值）之间查找回车换行符（由
        // kCRLF 和 kCRLF + 2 表示）
        const char* crlf = std::search(peek(), beginWrite(), kCRLF, kCRLF + 2);
        // 如果找到了回车换行符，则返回它在缓冲区中的指针；否则，返回 NULL
        return crlf == beginWrite() ? NULL : crlf;
    }

    const char* findCRLF(const char* start) const {
        const char* crlf = std::search(start, beginWrite(), kCRLF, kCRLF + 2);
        return crlf == beginWrite() ? NULL : crlf;
    }

    // 用于从缓冲区中取出数据直到指定位置
    // 接受一个 const char* 类型的参数
    // end，表示要取出数据的结束位置。通过计算要取出的数据长度（end-peek()），
    // 调用 retrieve() 函数将该长度的数据从缓冲区中取出。
    void retrieveUntil(const char* end) { retrieve(end - peek()); }

    char* beginWrite() { return begin() + writerIndex_; }

    const char* beginWrite() const { return begin() + writerIndex_; }

    // ssize表示有符号数据
    // 从fd上读取数据
    ssize_t readFd(int fd, int* saveErrno);
    // 通过fd发送数据
    ssize_t writeFd(int fd, int* saveErrno);

private:
    char* begin() {
        // it.operator*()
        // 解引用后取地址
        return &*buffer_
                     .begin();  // vector底层数组首元素的地址，也就是数组的起始地址
    }

    const char* begin() const { return &*buffer_.begin(); }

    void makeSpace(size_t len) {
        // 所有剩下可写的还是不够
        if (writableBytes() + prependableBytes() < len + kCheapPrepend) {
            // 至少有len的空间有的写
            buffer_.resize(writerIndex_ + len);
        } else {
            // 可读的数据
            size_t readalbe = readableBytes();
            // 可读但是还没用的空间，拷贝到begin()+kCheapPrepend
            // 这样把前面已经读过的数据的空间覆盖掉了
            std::copy(begin() + readerIndex_, begin() + writerIndex_,
                      begin() + kCheapPrepend);
            readerIndex_ = kCheapPrepend;
            writerIndex_ = readerIndex_ + readalbe;
        }
    }

    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;

    static const char kCRLF[];
};
