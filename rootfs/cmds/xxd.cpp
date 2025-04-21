#include "commands.h"
#include "kernel/syscall_user.h"
#include "lib/debug.h"
#include "utils.h"

void print_hex_line(const unsigned char* data, int size, int offset) {
    char line[80];
    char* ptr = line;
    
    // Print offset
    ptr += format_string(ptr, 8, "%08x: ", offset);
    
    // Print hex values
    for (int i = 0; i < 16; i++) {
        if (i < size) {
            ptr += format_string(ptr, 4, "%02x ", data[i]);
        } else {
            ptr += format_string(ptr, 4, "   ");
        }
        if (i == 7) {
            *ptr++ = ' ';
        }
    }
    
    // Print ASCII representation
    *ptr++ = ' ';
    *ptr++ = '|';
    for (int i = 0; i < size; i++) {
        if (data[i] >= 32 && data[i] <= 126) {
            *ptr++ = data[i];
        } else {
            *ptr++ = '.';
        }
    }
    *ptr++ = '|';
    *ptr++ = '\n';
    
    syscall_write(1, line, ptr - line);
}

void cmd_xxd(int argc, char* argv[]) {
    if (argc < 2) {
        log_debug("xxd: missing file operand\n");
        return;
    }
    
    const char* path = argv[1];
    int fd = syscall_open(path);
    if (fd < 0) {
        log_debug("xxd: cannot open '%s'\n", path);
        return;
    }
    
    unsigned char buf[16];
    int offset = 0;
    int n;
    
    while ((n = syscall_read(fd, buf, sizeof(buf))) > 0) {
        print_hex_line(buf, n, offset);
        offset += n;
    }
    
    syscall_close(fd);
}

REGISTER_COMMAND("xxd", cmd_xxd, "Make a hexdump");