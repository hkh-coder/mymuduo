#pragma once

#include <algorithm>
#include <string>
#include <vector>

// 服务器端接收数据使用的缓冲区类Buffer
class Buffer
{
public:
    // buffer缓冲区起始头部大小
    static const size_t KCheapPrepend = 8;
    // buffer缓冲区起始大小
    static const size_t KInitialSize = 1024;

    explicit Buffer(size_t initialSize = KInitialSize)
        : buffer_(KCheapPrepend + initialSize)
        , readIndex_(KCheapPrepend)
        , writeIndex_(KCheapPrepend)
    {
    }

    // 缓冲区中可读数据的长度
    size_t readableBytes() const { return writeIndex_ - readIndex_; }

    // 缓冲区中可写的大小
    size_t writableBytes() const { return buffer_.size() - writeIndex_; }

    //
    size_t prependableBytes() const { return readIndex_; }

    // 返回缓冲区中可读数据的起始地址
    const char *peek() const { return begin() + readIndex_; }

    // 调整readIndex_的位置
    void retrieve(size_t len)
    {
        if (len < readableBytes())
        { // 读取小于可读数据长度的len数据，
            // 就将readIndex_的位置向后调整
            readIndex_ + len;
        }
        else
        { // 读取所有的数据后将readIndex_和writeIndex_复位
            retrieveAll();
        }
    }

    // 将readIndex_和writeIndex_复位
    void retrieveAll()
    {
        readIndex_ = KCheapPrepend;
        writeIndex_ = KCheapPrepend;
    }

    std::string retrieveAllAsString()
    {
        // 读取所有可读的数据
        return retrieveAsString(readableBytes());
    }

    // 读取len长度的数据
    std::string retrieveAsString(size_t len)
    {
        std::string result(peek(), len);
        retrieve(len); // 调整readIndex_的位置
        return result; // 返回读取结果
    }

    // 需要写入的数据长度len和缓冲区中可用的长度比较
    void ensureWritableBytes(size_t len)
    {
        if (writableBytes() < len)
        {
            // 扩容buffer
            makeSpace(len);
        }
    }

    char* beginWrite()
    {
        return begin() + writeIndex_;
    }

    const char* beginWrite() const
    {
        return begin() + writeIndex_;
    }

    // 向缓冲区中写入数据
    void append(const char *data, size_t len)
    {
        ensureWritableBytes(len);
        std::copy(data, data + len, beginWrite());
        writeIndex_ += len;
    }

    // 从fd上读取数据
    ssize_t readFd(int fd, int *savedErrno);
    // 向fd上写数据
    ssize_t writeFd(int fd,int *savedErrno);
private:
    // 返回缓冲区起始地址的下标
    char *begin() { return &*buffer_.begin(); }
    const char *begin() const { return &*buffer_.begin(); }

    // 扩容buffer缓冲区大小
    void makeSpace(size_t len)
    {
        // 可写长度 + 起始位置到readIndex_ 的长度都不够
        if (writableBytes() + prependableBytes() < len + KCheapPrepend)
        {
            buffer_.resize(writeIndex_ + len); // 扩容
        }
        else
        {
            // 调整未被读走的数据的位置，
            size_t readable = readableBytes();
            std::copy(begin() + readIndex_,
                      begin() + writeIndex_,
                      begin() + KCheapPrepend);
            readIndex_ = KCheapPrepend;
            writeIndex_ = readIndex_ + readable;
        }
    }

    std::vector<char> buffer_;
    size_t readIndex_;
    size_t writeIndex_;
};