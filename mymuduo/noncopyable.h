#pragma once // 防止头文件被重复包含

/*
 * noncopyable -> 不能复制
 * 在muduo中大部分类都是以私有的方式继承noncopyable
 * 因为其将拷贝和赋值被delete掉，因此派生类都时无法拷贝和赋值操作
 * 但是构造和析构都是默认实现，因此派胜利构造和析构都是没有影响的
 * 比较优秀的设计，利用了派生类和基类的关系
 */

class noncpoyable
{
public:
    // delete掉拷贝构造函数
    noncpoyable(const noncpoyable &) = delete;
    // delete掉赋值运算符函数
    noncpoyable &operator=(const noncpoyable &) = delete;

protected:
    // 构造和析构都实现成默认的
    noncpoyable() = default;
    ~noncpoyable() = default;
};