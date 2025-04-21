#include "utils.h"
#include <cstdarg>
#include <kernel/syscall_user.h>

#include <lib/time.h>

// void log(const char* format, ...)
//// {
////     va_list args;
////     va_start(args, format);
////     char buf[1024];
////     format_string(buf, sizeof(buf), format, args);
////     va_end(args);
////     syscall_log(buf, 0);
//// }

void printf(const char* format, ...)
{
    va_list args;

    va_start(args, format);
    char buf[1024];
    auto len = format_string(buf, sizeof(buf), format, args);
    va_end(args);
    syscall_write(1, buf, len);
}

// utils.cpp
int sformat(char* buf, size_t bufsize, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int n = format_string(buf, bufsize, fmt, args);
    va_end(args);
    return n;
}
int format_string(char* buf, size_t bufsize, const char* fmt, va_list args)
{
    size_t i = 0;
    for(; *fmt && i + 1 < bufsize; ++fmt) {
        if(*fmt != '%') {
            buf[i++] = *fmt;
            continue;
        }
        ++fmt;
        if(*fmt == 'd') {
            int val = va_arg(args, int);
            char tmp[32];
            int n = 0, neg = 0;
            if(val < 0) {
                neg = 1;
                val = -val;
            }
            do {
                tmp[n++] = '0' + (val % 10);
                val /= 10;
            } while(val && n < 30);
            if(neg && n < 31)
                tmp[n++] = '-';
            while(n-- && i + 1 < bufsize)
                buf[i++] = tmp[n];
        } else if(*fmt == 'x') {
            unsigned val = va_arg(args, unsigned);
            char tmp[32];
            int n = 0;
            do {
                char c = (val % 16);
                tmp[n++] = c < 10 ? '0' + c : 'a' + c - 10;
                val /= 16;
            } while(val && n < 30);
            while(n-- && i + 1 < bufsize)
                buf[i++] = tmp[n];
        } else if(*fmt == 's') {
            const char* s = va_arg(args, const char*);
            while(*s && i + 1 < bufsize)
                buf[i++] = *s++;
        } else if(*fmt == 'c') {
            char c = (char)va_arg(args, int);
            if(i + 1 < bufsize)
                buf[i++] = c;
        } else if(*fmt == '%') {
            buf[i++] = '%';
        } else {
            // 不支持的格式，原样输出
            buf[i++] = '%';
            if(i + 1 < bufsize)
                buf[i++] = *fmt;
        }
    }
    buf[i] = '\0';
    return (int)i;
}

// 实现 atoi 函数
int atoi(char* input)
{
    int result = 0;
    int sign = 1;
    // 跳过前导空格
    while(*input == ' ') {
        input++;
    }
    // 处理符号
    if(*input == '-') {
        sign = -1;
        input++;
    } else if(*input == '+') {
        input++;
    }
    // 转换数字字符为整数
    while(*input >= '0' && *input <= '9') {
        result = result * 10 + (*input - '0');
        input++;
    }
    return sign * result;
}
void scanf(const char* format, ...)
{
    va_list args;
    va_start(args, format);

    char buf[1024];
    // 读取输入数据
    size_t bytes_read = syscall_read(0, buf, sizeof(buf) - 1);
    if(bytes_read > 0) {
        buf[bytes_read] = '\0'; // 确保字符串以 '\0' 结尾
    }

    const char* p = format;
    char* input = buf;
    while(*p) {
        if(*p == '%') {
            p++;
            switch(*p) {
                case 'd': {
                    int* num = va_arg(args, int*);
                    *num = atoi(input);
                    // 跳过已处理的数字字符
                    while(*input >= '0' && *input <= '9') {
                        input++;
                    }
                    break;
                }
                case 's': {
                    char* str = va_arg(args, char*);
                    // 复制字符串直到遇到空格或换行符
                    while(*input != ' ' && *input != '\n' && *input != '\0') {
                        *str++ = *input++;
                    }
                    *str = '\0';
                    break;
                }
                // 可以根据需要添加更多的格式说明符处理
                default:
                    break;
            }
            p++;
        } else {
            if(*p == *input) {
                input++;
            }
            p++;
        }
    }

    va_end(args);
}

void my_sleep(unsigned int ms)
{
    struct timespec req = {0, (long)ms * 1000000}; // 将毫秒转换为纳秒
    struct timespec rem;
    syscall_nanosleep(&req, &rem);
}
