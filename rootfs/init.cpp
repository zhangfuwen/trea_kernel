#define NO_PID 1
#include "kernel/syscall_user.h"
#include "lib/time.h"

#include <lib/debug.h>

const char* filename = "/dev/console";

// // 实现一个delay函数
// void delay(int ms) {
//     for (int i = 0; i < ms * 1000; i++) {
//         asm volatile("nop");
//     }
// }

int main()
{
    //   debug_debug("init: Hello, %x\n", filename);

    // debug_debug("init: open console!\n");
    // syscall_fork();
    // syscall_fork();
    // syscall_exit(NO_PID);

    char buf[1024] = "hello world!\n";
    while(true) {
        int pid = syscall_getpid();
        format_string(buf, 1024, "pid: %d\n", pid);
        syscall_write(2, buf, strlen(buf));
        struct timespec ts = {
            .tv_sec = 0,
            .tv_nsec = 1000 * 1000 * 1000,
        };
        syscall_nanosleep(&ts, &ts);
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        // asm volatile("hlt");
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        // serial_puts("hello world!\n");
    }
    // debug_debug("write to console: %d\n", fd);
    //  delay(1000*30);

    for(int i = 0; i < 1000; i++) {
        // debug_debug("init: open console! %d\n", i);
    }
    return 9;
}