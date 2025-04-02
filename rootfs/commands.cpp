#include "commands.h"
#include "utils.h"

#define MAX_COMMANDS 16

static Command commands[MAX_COMMANDS];
static int num_commands = 0;

void register_command(const Command& cmd) {
    // debug_debug("register command: %s\n", cmd.name);
    if (num_commands >= MAX_COMMANDS) {
        debug_debug("Cannot register command '%s': command table full\n", "asdfasd");
        debug_debug("Cannot register command '%s': command table full\n", cmd.name);
        return;
    }
    
    // 检查重复注册
    for (int i = 0; i < num_commands; i++) {
        if (strcmp(commands[i].name, cmd.name) == 0) {
            debug_debug("Command '%s' already registered\n", cmd.name);
            return;
        }
    }
    
    commands[num_commands++] = cmd;
}

void execute_command(const char* name, int argc, char* argv[]) {
    debug_debug("num_commands: %d\n", num_commands);
    for (int i = 0; i < num_commands; i++) {
        debug_debug("compare command '%s', '%s'\n", name, commands[i].name);
        if (strcmp(commands[i].name, name) == 0) {
            commands[i].handler(argc, argv);
            return;
        }
    }
    debug_debug("unknown command: %s\n", name);
}