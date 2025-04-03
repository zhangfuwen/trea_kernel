#include "commands.h"
#include "kernel/syscall_user.h"
#include "lib/debug.h"

void cmd_cd(int argc, char* argv[]) {
    if (argc < 2) {
        debug_debug("cd: missing directory operand\n");
        return;
    }

    const char* path = argv[1];
    if (syscall_chdir(path) < 0) {
        debug_debug("cd: cannot change directory to '%s'\n", path);
        return;
    }
}

REGISTER_COMMAND("cd", cmd_cd, "Change the current directory");