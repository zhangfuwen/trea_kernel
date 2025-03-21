#include "kernel/syscall_user.h"
#include "lib/debug.h"
const char * filename = "/dev/console";
int my_main() {
    debug_debug("init: Hello, %x\n", filename);

    debug_debug("init: open console!\n");
    int fd = syscall_open(filename);

    debug_debug("write to console: %d\n", fd);
    //for(int i = 0; i< 100;i++) {
    syscall_write(fd, filename, 14);
    //}
    return 0;
}