#pragma once

#include <string>

#include "noncopyable.h"

/*
 * 定义日志的级别：
 *          1、INFO 正常的流程输出
 *          2、ERROR 一些错误，但是不影响程序运行
 *          3、FATAL 致命的错误
 *          4、DEBUG 调试信息
 */
// 日志级别
enum LogLevel
{
    INFO,  // 普通信息
    ERROR, // 错误信息
    FATAL, // core信息
    DEBUG, // 调试信息
};

/*
 * 使用宏来调用日志，方便一些
 */

#define LOG_INFO(LogmsgFormat,...) \
do\
{ \
    Logger &logger = Logger::instance(); \
    logger.setLogLevel(INFO);\
    char buf[1024] = {0}; \
    snprintf(buf,1024,LogmsgFormat,## __VA_ARGS__);\
    logger.log(buf);\
} while (0)


#define LOG_ERROR(LogmsgFormat, ...) \
do\
{ \
    Logger &logger = Logger::instance(); \
    logger.setLogLevel(ERROR);\
    char buf[1024] = {0}; \
    snprintf(buf,1024,LogmsgFormat,## __VA_ARGS__);\
    logger.log(buf);\
} while (0)


#define LOG_FATAL(LogmsgFormat,...) \
do\
{ \
    Logger &logger = Logger::instance(); \
    logger.setLogLevel(FATAL);\
    char buf[1024] = {0}; \
    snprintf(buf,1024,LogmsgFormat,## __VA_ARGS__);\
    logger.log(buf);\
    exit(-1);\
} while (0)


// 因为dubug信息比较多，因此只用用户需要才打印，并且需要用户定义MUDUODEBUG宏才可以
#ifdef MUDEBUG
#define LOG_DEBUG(LogmsgFormat, ...) \
do\
{ \
    Logger &logger = Logger::instance(); \
    logger.setLogLevel(DEBUG);\
    char buf[1024] = {0}; \
    snprintf(buf,1024,LogmsgFormat,## __VA_ARGS__);\
    logger.log(buf);\
} while (0)

#else
    // 如果没有定义MUDUODEBUG就不打印任何调试信息
    #define LOG_DEBUG(LogmsgFormat, ...)
#endif

// 实现为单例模式
class Logger : noncpoyable
{
public:
    // 获取唯一的实例对象
    static Logger &instance();

    // 设置日志级别
    void setLogLevel(int level);

    // 写日志 打印格式：[级别信息] time : msg
    void log(std::string msg);

private:
    // 单例模式将构造私有化
    Logger() {}

    ///////////////成员变量///////////////
    int loglevel_; // 表示日志级别
};