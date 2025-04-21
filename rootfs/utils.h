#ifndef UTILS_H
#define UTILS_H

#include <cstdarg>
#include <cstddef>
#include <cstdint>
// 打印字符串
// void log(const char* format, ...);
void printf(const char* format, ...);
void scanf(const char* format, ...);
int atoi(char* input);
int format_string(char* buf, size_t bufsize, const char* fmt, va_list args);
int sformat(char* buf, size_t bufsize, const char* fmt, ...);

// 暂停当前进程指定的时间（毫秒）
void my_sleep(unsigned int ms);

#endif //UTILS_H
