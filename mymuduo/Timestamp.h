#pragma once
#include <iostream>
#include <string>
class Timestamp
{
public:
    Timestamp();
    // 带参数的构造函数，最好带上explicit,防止隐式转换
    explicit Timestamp(int64_t microSecondsSinceEpoch);
    
    // 获取当前时间
    static Timestamp now();

    // 将时间转换为字符串
    std::string toString()const; 
private:
    // 记录时间的64位整型
    int64_t microSecondsSinceEpoch_; 
};