#define NO_PID
#include "kernel/syscall_user.h"
#include "lib/string.h"
#include "utils.h"
#include "commands.h"


// 最大命令长度
#define MAX_CMD_LEN 256
#define MAX_ARGS 16

using namespace kernel;

void *operator new[](size_t size) {
    static char buffer[MAX_CMD_LEN];
    return buffer;
}

void cmd_help(int argc, char *argv[]) {
    printf("Usage: help [command]\n");
    printf("Available commands:\n");
    for (int i = 0; i < num_commands; i++) {
        printf("  %s %s\n", commands[i].name, commands[i].description);
    }
    // return 0;
}

// 解析命令行参数
int parse_cmd(char* cmd, char* argv[]) {
    int argc = 0;
    char* token = cmd;
    
    // 跳过开头的空格
    while (*token && *token == ' ') token++;
    
    while (*token && argc < MAX_ARGS) {
        argv[argc++] = token;
        
        // 找到下一个空格
        while (*token && *token != ' ') token++;
        
        // 如果找到空格，将其替换为\0并移动到下一个字符
        if (*token) {
            *token++ = '\0';
            while (*token && *token == ' ') token++;
        }
    }
    
    argv[argc] = nullptr;
    return argc;
}




int main() {
    char cmd_buf[MAX_CMD_LEN];
    char* argv[MAX_ARGS + 1];

#define EXTERN_REGISTER(name, desc) \
    extern void cmd_##name(int argc, char* argv[]); \
    register_command({#name, cmd_##name, desc})

    extern void cmd_cat(int argc, char* argv[]);
    extern void cmd_cd(int argc, char* argv[]);
    extern void cmd_echo(int argc, char* argv[]);
    extern void cmd_find(int argc, char* argv[]);
    extern void cmd_ls(int argc, char* argv[]);
    extern void cmd_rm(int argc, char* argv[]);
    extern void cmd_pwd(int argc, char* argv[]);
    extern void cmd_xxd(int argc, char* argv[]);
    // debug_debug("xxxxxxxxxxxxxxxxxxxxxx\n");
    // cmd_ls(0, nullptr);

    register_command({"cat", cmd_cat, "Concatenate and print files"});
    register_command({"cd", cmd_cd, "Concatenate and print files"});
    EXTERN_REGISTER(echo, "print message");
    EXTERN_REGISTER(find, "find files");
    register_command({"ls", cmd_ls, "List directory contents"});
    register_command({"rm", cmd_rm, "Remove file"});
    register_command({"pwd", cmd_pwd, "Concatenate and print files"});
    EXTERN_REGISTER(xxd, "print in hex format");
    EXTERN_REGISTER(help, "print help message");


    while (true) {

        // 显示提示符
        const char* prompt = "shell> ";
        printf(prompt);
        int n = 0;
        while(true) {
            int ret = syscall_read(0, cmd_buf + n, sizeof(cmd_buf));
            if(ret > 0 ) {
                // debug_debug("ret:%d, n:%d, cmd[%x]", ret, n, cmd_buf[n]);
                if (ret == 1 && cmd_buf[n] == '\b') {
                    if (n > 0) {
                        n--;
                        printf("\b \b", 3);
                    }
                    continue;
                } else {
                    printf("%c", *(char*)(cmd_buf + n));
                    n += ret;
                }
            }
            my_sleep(1000);

            if (n > 0 && (cmd_buf[n-1] == '\n' || cmd_buf[n-1] == '\r')) {
                cmd_buf[n-1] = '\0';
                n = 0;
                break;
            }
            if(n>=MAX_CMD_LEN) {
                cmd_buf[n-1] = '\0';
                n = 0;
                break;
            }
        }

        // 解析命令
        int argc = parse_cmd(cmd_buf, argv);
        printf("argc %d, argv[0]:%s\n", argc,argv[0]);
        if (argc == 0) continue;

        // 执行命令
        if (strcmp(argv[0], "exit") == 0) {
            break;
        }
        else {
            // 使用新的命令框架执行命令
            execute_command(argv[0], argc, argv);
        }
    }

    return 0;
}