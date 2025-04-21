#include "commands.h"
#include "utils.h"

void cmd_echo(int argc, char* argv[]) {
    if (argc < 2) {
        const char* newline = "\n";
        printf(newline);
        return;
    }

    for (int i = 1; i < argc; i++) {
        printf( argv[i]);
        if (i < argc - 1) {
            const char* space = " ";
            printf( space);
        }
    }
    const char* newline = "\n";
    printf(newline);
}

REGISTER_COMMAND("echo", cmd_echo, "Display a line of text");