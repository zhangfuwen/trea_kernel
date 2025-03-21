#ifndef SYSCALL_USER_H
#define SYSCALL_USER_H

#include <stdint.h>
#include <unistd.h>

// 系统调用号
#define SYS_FORK 1
#define SYS_EXEC 2
#define SYS_OPEN 3
#define SYS_READ 4
#define SYS_WRITE 5
#define SYS_CLOSE 6
#define SYS_SEEK 7

// 系统调用接口
extern "C" {
    // fork系统调用
    inline int syscall_fork() {
        int ret;
        asm volatile(
            "int $0x80"
            : "=a"(ret)
            : "a"(SYS_FORK)
            : "memory"
        );
        return ret;
    }

    // exec系统调用
    inline int syscall_exec(const char* path, char* const argv[]) {
        int ret;
        asm volatile(
            "int $0x80"
            : "=a"(ret)
            : "a"(SYS_EXEC), "b"(path), "c"(argv)
            : "memory"
        );
        return ret;
    }

    // open系统调用
    inline int syscall_open(const char* path) {
        int ret;
        int c = 5;
        int d = 6;
        asm volatile(
            "int $0x80"
            : "=a"(ret)
            : "a"(SYS_OPEN), "b"(path)
            : "memory"
        );
        return ret;
    }

    // read系统调用
    inline int syscall_read(int fd, void* buffer, size_t size) {
        int ret;
        asm volatile(
            "int $0x80"
            : "=a"(ret)
            : "a"(SYS_READ), "b"(fd), "c"(buffer), "d"(size)
            : "memory"
        );
        return ret;
    }

    // write系统调用
    inline int syscall_write(int fd, const void* buffer, size_t size) {
        int ret;
        asm volatile(
            "int $0x80"
            : "=a"(ret)
            : "a"(SYS_WRITE), "b"(fd), "c"(buffer), "d"(size)
            : "memory"
        );
        return ret;
    }

    // close系统调用
    inline int syscall_close(int fd) {
        int ret;
        asm volatile(
            "int $0x80"
            : "=a"(ret)
            : "a"(SYS_CLOSE), "b"(fd)
            : "memory"
        );
        return ret;
    }

    // seek系统调用
    inline int syscall_seek(int fd, size_t offset) {
        int ret;
        asm volatile(
            "int $0x80"
            : "=a"(ret)
            : "a"(SYS_SEEK), "b"(fd), "c"(offset)
            : "memory"
        );
        return ret;
    }
}

#endif // SYSCALL_USER_H