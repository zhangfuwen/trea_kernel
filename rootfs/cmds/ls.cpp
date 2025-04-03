#include "commands.h"
#include "kernel/syscall_user.h"
#include "kernel/dirent.h"
#include "lib/string.h"
#include "lib/debug.h"
#include "utils.h"

using namespace kernel;
void cmd_ls(int argc, char* argv[]) {
    const char* path = argc > 1 ? argv[1] : ".";
    
    kernel::FileAttribute attr;
    if (syscall_stat(path, &attr) < 0) {
        debug_debug("ls: cannot access '%s'\n", path);
        return;
    }

    debug_debug("ls: path '%s'\n", path);
    debug_debug("ls: attr.mode 0x%x\n", attr.mode);
    if (attr.type != kernel::FT_DIR) {
        char buf[256];
        format_string(buf, sizeof(buf), "%s\t%d bytes\n", path, attr.size);
        syscall_write(1, buf, strlen(buf));
        return;
    }

    int fd = syscall_open(path);
    if (fd < 0) {
        debug_debug("ls: cannot open directory '%s'\n", path);
        return;
    }
    debug_debug("ls: open directory '%s'\n", path);

    char title[256];
    format_string(title, sizeof(title), "Directory listing of %s:\n", path);
    syscall_write(1, title, strlen(title));

    char dirent_buf[1024];
    uint32_t pos = 0;
    int bytes_read;
    
    while (true) {
        bytes_read = syscall_getdents(fd, dirent_buf, sizeof(dirent_buf), &pos);
        if (bytes_read < 0) {
            debug_debug("ls: cannot read directory '%s'\n", path);
            break;
        } else {
            debug_debug("ls: read:%d, %s\n", bytes_read, dirent_buf);
        }
        char* ptr = dirent_buf;
        while (ptr < dirent_buf + bytes_read) {
            dirent* entry = (dirent*)ptr;
            
            char full_path[256];
            format_string(full_path, sizeof(full_path), 
                strcmp(path, "/") == 0 ? "/%s\n" : "%s/%s\n", entry->d_name);
            syscall_write(1, full_path, strlen(full_path));
            
            // kernel::FileAttribute file_attr;
            // syscall_stat(full_path, &file_attr);
            //
            // char type_char = ' ';
            // switch (entry->d_type) {
            //     case 2: type_char = 'd'; break;
            //     case 1: type_char = '-'; break;
            //     default: type_char = '?';
            // }
            //
            // char line[256];
            // format_string(line, sizeof(line), "%c %s\t%d bytes\n",
            //              type_char, entry->d_name, file_attr.size);
            // syscall_write(1, line, strlen(line));
            
            ptr += entry->d_reclen;
        }
        if (bytes_read < sizeof(dirent_buf)) {
            break;
        }
    }
    syscall_close(fd);
}

REGISTER_COMMAND("ls", cmd_ls, "List directory contents");