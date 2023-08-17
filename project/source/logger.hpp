#ifndef _M_LOGGER_H__
#define _M_LOGGER_H__

#include <stdio.h>
#include <time.h>

#define INF 0
#define DEBUG 1
#define ERR 2
#define DEFAULT_LOG_LEVEL DEBUG // 只有大于默认等级的日志信息才会被输出

#define LOG(level, format, ...)                                                             \
    do                                                                                      \
    {                                                                                       \
        if (level < DEFAULT_LOG_LEVEL)                                                      \
            break;                                                                          \
        time_t t = time(NULL);                                                              \
        struct tm *lt = localtime(&t);                                                      \
        char buf[32] = {0};                                                                 \
        strftime(buf, 31, "%H:%M:%s", lt);                                                  \
        fprintf(stdout, "[%s %s:%d] " format "\n", buf, __FILE__, __LINE__, ##__VA_ARGS__); \
    } while (0)
/*
    ##__VA_ARGS__前面加上##的作用是：当可变参数的个数为0时，这里的##可以把把前面多余的","去掉,否则会编译出错。
*/
#define ILOG(format, ...) LOG(INF, format, ##__VA_ARGS__)
#define DLOG(format, ...) LOG(DEBUG, format, ##__VA_ARGS__)
#define ELOG(format, ...) LOG(ERR, format, ##__VA_ARGS__)

#endif // !_M_LOGGER_H__