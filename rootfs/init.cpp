#define NO_PID
#include "kernel/syscall_user.h"
#include "lib/string.h"
#include "lib/debug.h"
#include "kernel/dirent.h"
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
    //自动注册命令
    // extern Command __cmd_table_start;
    // extern Command __cmd_table_end;
    // debug_debug("command table start: 0x%x, end: 0x%x\n", &__cmd_table_start, &__cmd_table_end);
    // for (Command* cmd = &__cmd_table_start; cmd < &__cmd_table_end; cmd++) {
    //
    //     register_command(*cmd);
    // }

    extern void cmd_ls(int argc, char* argv[]);
    extern void cmd_rm(int argc, char* argv[]);
    extern void cmd_cat(int argc, char* argv[]);
    // debug_debug("xxxxxxxxxxxxxxxxxxxxxx\n");
    // cmd_ls(0, nullptr);

    register_command({"ls", cmd_ls, "List directory contents"});
    register_command({"rm", cmd_rm, "Remove file"});
    register_command({"cat", cmd_cat, "Concatenate and print files"});

    set_log_output_handler(my_log_output_interface);

    while (true) {

        // 显示提示符
        const char* prompt = "shell> ";
        syscall_write(1, prompt, strlen(prompt));
        // printf("%s", prompt);

        int n = 0;
        while(true) {
            int ret = syscall_read(0, cmd_buf + n, sizeof(cmd_buf));
            if(ret > 0 ) {
                // debug_debug("ret:%d, n:%d, cmd[%x]", ret, n, cmd_buf[n]);
                if (ret == 1 && cmd_buf[n] == '\b') {
                    if (n > 0) {
                        n--;
                        syscall_write(1, "\b \b", 3);
                    }
                    continue;
                }
                syscall_write(1, cmd_buf + n, ret);
                n += ret;
            }

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