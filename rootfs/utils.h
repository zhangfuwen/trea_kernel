#ifndef UTILS_H
#define UTILS_H

#include "kernel/syscall_user.h"
extern LogOutputInterface my_log_output_interface;
// 打印字符串
void log(const char* format, ...);
void printf(const char* format, ...);
void scanf(const char* format, ...);
int atoi(char* input);




#endif //UTILS_H
