#include "kernel/syscall_user.h"
int main() {
    int fd = syscall_open("/dev/console");
    syscall_write(fd, "init: Hello, world!\n", 14);
    return 0;
}