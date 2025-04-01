#ifndef SYSCALL_USER_H
#define SYSCALL_USER_H

#include "syscall.h"

#include <stdint.h>
#include <unistd.h>

// 系统调用接口
extern "C" {
// fork系统调用
inline int syscall_fork()
{
    int ret;
    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_FORK) : "memory");
    return ret;
}

// exec系统调用
inline int syscall_exec(const char* path, char* const argv[])
{
    int ret;
    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_EXEC), "b"(path), "c"(argv) : "memory");
    return ret;
}

// open系统调用
inline int syscall_open(const char* path)
{
    int ret;
    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_OPEN), "b"(path) : "memory");
    return ret;
}

// read系统调用
inline int syscall_read(int fd, void* buffer, size_t size)
{
    int ret;
    asm volatile("int $0x80"
                 : "=a"(ret)
                 : "a"(SYS_READ), "b"(fd), "c"(buffer), "d"(size)
                 : "memory");
    return ret;
}

// write系统调用
inline int syscall_write(int fd, const void* buffer, size_t size)
{
    int ret;
    asm volatile("int $0x80"
                 : "=a"(ret)
                 : "a"(SYS_WRITE), "b"(fd), "c"(buffer), "d"(size)
                 : "memory");
    return ret;
}

// close系统调用
inline int syscall_close(int fd)
{
    int ret;
    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_CLOSE), "b"(fd) : "memory");
    return ret;
}

// seek系统调用
inline int syscall_seek(int fd, size_t offset)
{
    int ret;
    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_SEEK), "b"(fd), "c"(offset) : "memory");
    return ret;
}

// exit系统调用
inline void syscall_exit(int status)
{
    asm volatile("int $0x80" : : "a"(SYS_EXIT), "b"(status) : "memory");
    while(1)
        ; // 防止返回
}

// 添加用户空间getpid封装
}
inline pid_t syscall_getpid() {
    uint32_t ret;
    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_GETPID));
    return ret;
}

inline int syscall_nanosleep(const struct timespec *req, struct timespec *rem) {
    int ret;
    asm volatile("int $0x80"
        : "=a"(ret)
        : "a"(SYS_NANOSLEEP), "b"((uint32_t)req), "c"((uint32_t)rem));
    return ret;
}

// stat系统调用
inline int syscall_stat(const char* path, kernel::FileAttribute* attr) {
    int ret;
    asm volatile("int $0x80"
        : "=a"(ret)
        : "a"(SYS_STAT), "b"((uint32_t)path), "c"((uint32_t)attr));
    return ret;
}

// mkdir系统调用
inline int syscall_mkdir(const char* path) {
    int ret;
    asm volatile("int $0x80"
        : "=a"(ret)
        : "a"(SYS_MKDIR), "b"((uint32_t)path));
    return ret;
}

// unlink系统调用
inline int syscall_unlink(const char* path) {
    int ret;
    asm volatile("int $0x80"
        : "=a"(ret)
        : "a"(SYS_UNLINK), "b"((uint32_t)path));
    return ret;
}

// rmdir系统调用
inline int syscall_rmdir(const char* path) {
    int ret;
    asm volatile("int $0x80"
        : "=a"(ret)
        : "a"(SYS_RMDIR), "b"((uint32_t)path));
    return ret;
}

#endif // SYSCALL_USER_H