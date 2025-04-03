#include "commands.h"
#include "kernel/syscall_user.h"
#include "lib/string.h"

void cmd_echo(int argc, char* argv[]) {
    if (argc < 2) {
        const char* newline = "\n";
        syscall_write(1, newline, 1);
        return;
    }

    for (int i = 1; i < argc; i++) {
        syscall_write(1, argv[i], strlen(argv[i]));
        if (i < argc - 1) {
            const char* space = " ";
            syscall_write(1, space, 1);
        }
    }
    const char* newline = "\n";
    syscall_write(1, newline, 1);
}

REGISTER_COMMAND("echo", cmd_echo, "Display a line of text");