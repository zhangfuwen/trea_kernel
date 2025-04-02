#include "commands.h"
#include "kernel/dirent.h"
#include "kernel/syscall_user.h"
#include "lib/debug.h"

void cmd_rm(int argc, char* argv[]) {
    if (argc < 2) {
        debug_debug("rm: missing operand\n");
        return;
    }
    
    const char* path = argv[1];
    if (syscall_unlink(path) < 0) {
        debug_debug("rm: cannot remove '%s'\n", path);
    }
}
__attribute__((constructor)) void rm_register() {
    register_command({
        .name = "rm",
        .handler = cmd_rm,
        .description = "Remove files or directories"
    });
}


REGISTER_COMMAND("rm", cmd_rm, "Remove files or directories");
