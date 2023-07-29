#include "Buffer.h"

#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>

/**
 * 从fd上读取数据 Poller工作在LT模式(数据没有读完的话，底层poller会不断地去上报)
 * Buffer缓冲区是有大小的！ 但是从fd上读数据的时候，却不知道tcp数据最终的大小
 */
ssize_t Buffer::readFd(int fd, int* saveErrno) {
    char extrabuf[65536] = {0};  // 栈上的内存空间 64K
    struct iovec vec[2];

    const size_t writable =
        writableBytes();  // 这是Buffer底层缓冲区剩余的可写空间大小
    // 可填就只用第一块
    // 第一块缓冲区
    vec[0].iov_base = begin() + writerIndex_;
    vec[0].iov_len = writable;

    // 第二块缓冲区
    // 不够填，就要把数据放在栈里面
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof extrabuf;

    const int iovcnt = (writable < sizeof extrabuf) ? 2 : 1;
    // 数组名vec本身就是地址
    // readv可以从文件描述符fd中读取数据到多个缓冲区中
    const ssize_t n = ::readv(fd, vec, iovcnt);
    if (n < 0) {
        *saveErrno = errno;
    } else if (n <= writable)  // Buffer的可写缓冲区已经够存储读出来的数据了
    {
        writerIndex_ += n;
    } else {
        // Buffer空间不够存，需要把溢出的部分（extrabuf）倒到Buffer中（会先触发扩容机制）
        // extrabuf里面也写入了数据
        writerIndex_ = buffer_.size();
        // Buffer已经存了writable个数据
        append(extrabuf,
               n - writable);  // writerIndex_开始写 n - writable大小的数据
    }

    return n;
}

ssize_t Buffer::writeFd(int fd, int* saveErrno) {
    ssize_t n = ::write(fd, peek(), readableBytes());
    if (n < 0) {
        // 出错了返回errno
        *saveErrno = errno;
    }
    return n;
}
