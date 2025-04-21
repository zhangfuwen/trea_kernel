#pragma once

#include "lib/string.h"

typedef void (*CommandHandler)(int argc, char* argv[]);

struct Command {
    const char* name;
    CommandHandler handler;
    const char* description;
};
#define MAX_COMMANDS 1000
extern Command commands[MAX_COMMANDS];
extern int num_commands;

void register_command(const Command& cmd);
void execute_command(const char* name, int argc, char* argv[]);

#define __REGISTER_COMMAND_IMPL(unique_id) \
    static Command __cmd_##unique_id __attribute__((used, section(".cmd_table"))) = 

#define REGISTER_COMMAND(name, handler, ...) \
    __REGISTER_COMMAND_IMPL(__COUNTER__) {name, handler, ##__VA_ARGS__};
