#include "commands.h"
#include "kernel/dirent.h"
#include "kernel/syscall_user.h"
#include "utils.h"

void cmd_cat(int argc, char* argv[]) {
    if (argc < 2) {
        printf("cat: missing operand\n");
        return;
    }
    
    const char* path = argv[1];
    int fd = syscall_open(path);
    if (fd < 0) {
        printf("cat: cannot open '%s'\n", path);
        return;
    }
    
    char buf[1024];
    int n;
    while ((n = syscall_read(fd, buf, sizeof(buf))) > 0) {
        syscall_write(1, buf, n);
    }
    
    syscall_close(fd);
}

REGISTER_COMMAND("cat", cmd_cat, "Concatenate and print files");