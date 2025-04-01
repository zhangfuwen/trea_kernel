#ifndef SYSCALL_H
#define SYSCALL_H

#include "process.h"

#include <stdint.h>

// 系统调用号
enum SyscallNumber {
    SYS_FORK = 1,
    SYS_EXEC = 2,
    SYS_OPEN = 3,
    SYS_READ = 4,
    SYS_WRITE = 5,
    SYS_CLOSE = 6,
    SYS_SEEK = 7,
    SYS_EXIT = 8,
    SYS_GETPID = 9,
    SYS_NANOSLEEP = 10,
    SYS_STAT = 11,
    SYS_MKDIR = 12,
    SYS_UNLINK = 13,
    SYS_RMDIR = 14,
};

// 系统调用处理函数类型
typedef int (*SyscallHandler)(uint32_t, uint32_t, uint32_t, uint32_t);

// 系统调用处理函数声明
int exitHandler(uint32_t status, uint32_t, uint32_t, uint32_t);
int execveHandler(uint32_t path_ptr, uint32_t argv_ptr, uint32_t envp_ptr, uint32_t);
int sys_execve(uint32_t path_ptr, uint32_t argv_ptr, uint32_t envp_ptr, ProcessControlBlock *pcb);
int getpid_handler(uint32_t a, uint32_t b, uint32_t c, uint32_t d);
int sys_getpid();
int statHandler(uint32_t path_ptr, uint32_t attr_ptr, uint32_t, uint32_t);
int mkdirHandler(uint32_t path_ptr, uint32_t, uint32_t, uint32_t);
int unlinkHandler(uint32_t path_ptr, uint32_t, uint32_t, uint32_t);
int rmdirHandler(uint32_t path_ptr, uint32_t, uint32_t, uint32_t);

// 系统调用管理器
class SyscallManager
{
public:
    static void init();
    static void registerHandler(uint32_t syscall_num, SyscallHandler handler);
    static int handleSyscall(
        uint32_t syscall_num, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4);

private:
    static SyscallHandler handlers[256];
    static void defaultHandler();
};

#endif // SYSCALL_H