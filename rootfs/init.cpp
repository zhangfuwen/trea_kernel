#include "kernel/syscall_user.h"
#define NO_PID 1
#include "lib/debug.h"
const char * filename = "/dev/console";

// 实现一个delay函数
void delay(int ms) {
    for (int i = 0; i < ms * 1000; i++) {
        asm volatile("nop");
    }
}

int my_main() {
 //   debug_debug("init: Hello, %x\n", filename);

    debug_debug("init: open console!\n");
   // int fd = syscall_open(filename);
    syscall_fork();
    syscall_fork();

    //debug_debug("write to console: %d\n", fd);
    delay(1000*30);

    for(int i = 0; i< 1000;i++) {
        debug_debug("init: open console! %d\n", i);
    }
    return 9;
}