#include "kernel/syscall_user.h"
#define NO_PID 1
#include <lib/serial.h>

#include "lib/debug.h"
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

    while(true) {
        // int fd = syscall_open(filename);
        //  if (fd < 0) {
        //      debug_err("Failed to open console!\n");
        //      return -1;
        //  } else {
        //      debug_debug("open console: %d\n", fd);
        //      char buf[1024] = "hello world!\n";
        //      int ret = syscall_write(fd, buf, 13);
        //      if (ret < 0) {
        //          debug_err("Failed to write to console!\n");
        //          return -1;
        //      }
        //  }
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        // serial_puts("hello world!\n");
    }
    // debug_debug("write to console: %d\n", fd);
    //  delay(1000*30);

    for(int i = 0; i < 1000; i++) {
        debug_debug("init: open console! %d\n", i);
    }
    return 9;
}