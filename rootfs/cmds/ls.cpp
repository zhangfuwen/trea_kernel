#include "commands.h"
#include "kernel/syscall_user.h"
#include "kernel/dirent.h"
#include "lib/string.h"
#include "utils.h"

using namespace kernel;
void cmd_ls(int argc, char* argv[]) {
    const char* path = argc > 1 ? argv[1] : ".";
    
    kernel::FileAttribute attr;
    if (syscall_stat(path, &attr) < 0) {
        printf("ls: cannot access '%s'\n", path);
        return;
    }

    // printf("ls: path '%s'\n", path);
    // printf("ls: attr.mode 0x%x\n", attr.mode);
    if (attr.type != kernel::FT_DIR) {
        char buf[256];
        sformat(buf, sizeof(buf), "%s\t%d bytes\n", path, attr.size);
        syscall_write(1, buf, strlen(buf));
        return;
    }

    int fd = syscall_open(path);
    if (fd < 0) {
        printf("ls: cannot open directory '%s'\n", path);
        return;
    }
    // printf("ls: open directory '%s'\n", path);

    char title[256];
    sformat(title, sizeof(title), "Directory listing of %s:\n", path);
    syscall_write(1, title, strlen(title));

    char dirent_buf[1024];
    uint32_t pos = 0;
    int bytes_read;
    
    while (true) {
        bytes_read = syscall_getdents(fd, dirent_buf, sizeof(dirent_buf), &pos);
        if (bytes_read <= 0) {
            break;
        }
        char* ptr = dirent_buf;
        while (ptr < dirent_buf + bytes_read) {
            struct dirent* entry = reinterpret_cast<struct dirent*>(ptr);
            syscall_write(1, entry->d_name, strlen(entry->d_name));
            syscall_write(1, "\n", 1);
            ptr += entry->d_reclen;
        }
    }
    syscall_close(fd);
}

REGISTER_COMMAND("ls", cmd_ls, "List directory contents");