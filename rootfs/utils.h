#ifndef UTILS_H
#define UTILS_H

#define NO_PID
#include "lib/debug.h"

#include "kernel/syscall_user.h"
extern LogOutputInterface my_log_output_interface;
// 打印字符串
void log(const char* format, ...);
void printf(const char* format, ...);
void scanf(const char* format, ...);
int atoi(char* input);

// 暂停当前进程指定的时间（毫秒）
void my_sleep(unsigned int ms);

#endif //UTILS_H
