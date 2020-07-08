#include "Buffer.h"
#include <sys/uio.h>
#include <unistd.h>

// 从fd上读取数据
ssize_t Buffer::readFd(int fd, int *savedErrno)
{
    char extrabuf[65536] = {0}; // 64K的栈上数组
    iovec vec[2];
    const size_t writebale = writableBytes();
    vec[0].iov_base = begin() + writeIndex_;
    vec[0].iov_len = writebale;
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof extrabuf;

    // 使用一个条件句，可以看出muduo最多读取64K的数据
    const int iovcnt = (writebale < sizeof extrabuf) ? 2 : 1;
    // readv可以指定多块不连续的内存，并将数据写入到这些内存中
    const ssize_t n = ::readv(fd, vec, iovcnt);
    if (n < 0)
    { // 出错并将错误带回
        *savedErrno = errno;
    }
    else if (n <= writebale)
    { // buffer本身的缓冲区够用
        writeIndex_ += n;
    }
    else
    { // buffer不够，使用exteabuf，再将extrabuf的数据写回到已经扩容后buffer中
        writeIndex_ = buffer_.size();
        append(extrabuf, n - writebale);
    }
    return n;
}

// 将缓冲区的数据写入fd
ssize_t Buffer::writeFd(int fd, int *savedErrno)
{
    size_t n = ::write(fd,peek(),readableBytes());
    if(n < 0)
    {
        *savedErrno = errno;
    }
    return n;
}