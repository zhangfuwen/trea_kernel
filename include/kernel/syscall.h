#ifndef SYSCALL_H
#define SYSCALL_H

#include "process.h"

#include <cstdint>

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
    SYS_GETDENTS = 15,
    SYS_LOG = 16,
    SYS_CHDIR = 17,
    SYS_PWD = 18,
    SYS_GETCWD = 19,
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
int getdentsHandler(uint32_t fd_num, uint32_t dirp, uint32_t count, uint32_t pos_ptr);
int sys_getdents(uint32_t fd_num, uint32_t dirp, uint32_t count, uint32_t pos_ptr);
void sys_log(const char* message, uint32_t len);
int logHandler(uint32_t message_ptr, uint32_t len, uint32_t, uint32_t);
int chdirHandler(uint32_t path_ptr, uint32_t, uint32_t, uint32_t);
int sys_chdir(const char* path);
int getcwdHandler(uint32_t buf_ptr, uint32_t size, uint32_t, uint32_t);
int sys_getcwd(char* buf, size_t size);

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