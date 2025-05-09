#include "commands.h"
#include "kernel/syscall_user.h"
#include "utils.h"

void cmd_cd(int argc, char* argv[]) {
    if (argc < 2) {
        printf("cd: missing directory operand\n");
        return;
    }

    const char* path = argv[1];
    if (syscall_chdir(path) < 0) {
        printf("cd: cannot change directory to '%s'\n", path);
        return;
    }
}

void cmd_pwd(int argc, char* argv[]) {
    char cwd[1024];
    if (syscall_pwd(cwd, sizeof(cwd)) < 0) {
        printf("pwd: cannot get current directory\n");
        return;
    }
    syscall_write(1, cwd, strlen(cwd));
    syscall_write(1, "\n", 1);
}

REGISTER_COMMAND("cd", cmd_cd, "Change the current directory");
// REGISTER_COMMAND("pwd", cmd_pwd, "Print the current directory");