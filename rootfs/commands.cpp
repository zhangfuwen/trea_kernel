#include "commands.h"
#include "utils.h"


Command commands[MAX_COMMANDS];
int num_commands = 0;

void register_command(const Command& cmd) {
    // debug_debug("register command: %s\n", cmd.name);
    if (num_commands >= MAX_COMMANDS) {
        printf("Cannot register command '%s': command table full\n", "asdfasd");
        printf("Cannot register command '%s': command table full\n", cmd.name);
        return;
    }
    
    // 检查重复注册
    for (int i = 0; i < num_commands; i++) {
        if (strcmp(commands[i].name, cmd.name) == 0) {
            printf("Command '%s' already registered\n", cmd.name);
            return;
        }
    }
    
    commands[num_commands++] = cmd;
}

void execute_command(const char* name, int argc, char* argv[]) {
    // printf("num_commands: %d\n", num_commands);
    for (int i = 0; i < num_commands; i++) {
        // printf("compare command '%s', '%s'\n", name, commands[i].name);
        if (strcmp(commands[i].name, name) == 0) {
            commands[i].handler(argc, argv);
            return;
        }
    }
    printf("unknown command: %s\n", name);
}