#include "commands.h"
#include "kernel/dirent.h"
#include "kernel/syscall_user.h"
#include "lib/debug.h"
#include "lib/string.h"
#include "utils.h"
// #include <cstring>

using namespace kernel;

void find_in_directory(const char* dir_path, const char* pattern) {
    int fd = syscall_open(dir_path);
    if (fd < 0) {
        debug_debug("find: cannot open directory '%s'\n", dir_path);
        return;
    }

    char dirent_buf[1024];
    uint32_t pos = 0;
    int bytes_read;

    while (true) {
        bytes_read = syscall_getdents(fd, dirent_buf, sizeof(dirent_buf), &pos);
        if (bytes_read < 0) {
            debug_debug("find: cannot read directory '%s'\n", dir_path);
            break;
        }

        char* ptr = dirent_buf;
        while (ptr < dirent_buf + bytes_read) {
            dirent* entry = (dirent*)ptr;
            
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                char full_path[256];
                format_string(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);

                if (pattern == nullptr || strstr(entry->d_name, pattern) != nullptr) {
                    syscall_write(1, full_path, strlen(full_path));
                    syscall_write(1, "\n", 1);
                }

                if (entry->d_type == 2) { // Directory
                    find_in_directory(full_path, pattern);
                }
            }
            
            ptr += entry->d_reclen;
        }

        if (bytes_read < (int)sizeof(dirent_buf)) {
            break;
        }
    }

    syscall_close(fd);
}

void cmd_find(int argc, char* argv[]) {
    const char* path = ".";
    const char* pattern = nullptr;

    if (argc > 1) {
        path = argv[1];
    }
    if (argc > 2) {
        pattern = argv[2];
    }

    find_in_directory(path, pattern);
}

REGISTER_COMMAND("find", cmd_find, "Search for files in a directory hierarchy");