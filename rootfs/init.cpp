#include "kernel/syscall_user.h"
#include "lib/debug.h"
int my_main() {
    debug_debug("init: Hello, world!\n");
    debug_debug("init: Hello, world!\n");
    debug_debug("init: open console!\n");
    int fd = syscall_open("/dev/console");
    debug_debug("write to console: %d\n", fd);
    for(int i = 0; i< 100;i++) {
        syscall_write(fd, "init: ----- Hello, world!\n", 14);
    }
    return 0;
}