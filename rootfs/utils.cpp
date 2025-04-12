#include "lib/string.h"
#include <kernel/syscall_user.h>
#include <lib/debug.h>

#include <lib/time.h>
LogOutputInterface my_log_output_interface {
  .print = [](LogLevel level, const char* message) {
    syscall_log(message, 0);
  }
};

void log(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    char buf[1024];
    format_string(buf, sizeof(buf), format, args);
    va_end(args);
    syscall_log(buf, 0);
}

void printf(const char* format, ...)
{
    va_list args;

    va_start(args, format);
    char buf[1024];
    format_string(buf, sizeof(buf), format, args);
    va_end(args);
    syscall_write(1, buf, strlen(buf));
}

// 实现 atoi 函数
int atoi(char* input) {
    int result = 0;
    int sign = 1;
    // 跳过前导空格
    while (*input == ' ') {
        input++;
    }
    // 处理符号
    if (*input == '-') {
        sign = -1;
        input++;
    } else if (*input == '+') {
        input++;
    }
    // 转换数字字符为整数
    while (*input >= '0' && *input <= '9') {
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
    if (bytes_read > 0) {
        buf[bytes_read] = '\0'; // 确保字符串以 '\0' 结尾
    }

    const char* p = format;
    char* input = buf;
    while (*p) {
        if (*p == '%') {
            p++;
            switch (*p) {
                case 'd': {
                    int* num = va_arg(args, int*);
                    *num = atoi(input);
                    // 跳过已处理的数字字符
                    while (*input >= '0' && *input <= '9') {
                        input++;
                    }
                    break;
                }
                case 's': {
                    char* str = va_arg(args, char*);
                    // 复制字符串直到遇到空格或换行符
                    while (*input != ' ' && *input != '\n' && *input != '\0') {
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
            if (*p == *input) {
                input++;
            }
            p++;
        }
    }

    va_end(args);

}

void my_sleep(unsigned int ms) {
    struct timespec req = {0, (long)ms * 1000000}; // 将毫秒转换为纳秒
    struct timespec rem;
    syscall_nanosleep(&req, &rem);
}