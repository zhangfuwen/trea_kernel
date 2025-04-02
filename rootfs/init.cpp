#define NO_PID
#include "kernel/syscall_user.h"
#include "lib/string.h"
#include "lib/debug.h"


// 最大命令长度
#define MAX_CMD_LEN 256
#define MAX_ARGS 16

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

// 执行ls命令
void cmd_ls(const char* path) {
    kernel::FileAttribute attr;
    if (syscall_stat(path, &attr) < 0) {
        debug_debug("ls: cannot access '%s'\n", path);
        return;
    }
    
    char buf[256];
    format_string(buf, sizeof(buf), "%s\n", path);
    syscall_write(1, buf, strlen(buf));
}

// 执行cat命令
void cmd_cat(const char* path) {
    int fd = syscall_open(path);
    if (fd < 0) {
        debug_debug("cat: cannot open '%s'\n", path);
        return;
    }
    
    char buf[1024];
    int n;
    while ((n = syscall_read(fd, buf, sizeof(buf))) > 0) {
        syscall_write(1, buf, n);
    }
    
    syscall_close(fd);
}

// 执行rm命令
void cmd_rm(const char* path) {
    if (syscall_unlink(path) < 0) {
        debug_debug("rm: cannot remove '%s'\n", path);
    }
}
LogOutputInterface my_log_output_interface {
    .print = [](LogLevel level, const char* message) {
        syscall_write(1, message, strlen(message));
    }
};

int main() {
    char cmd_buf[MAX_CMD_LEN];
    char* argv[MAX_ARGS + 1];

    set_log_output_handler(my_log_output_interface);
    
    while (true) {
        // 显示提示符
        const char* prompt = "shell> ";
        syscall_write(1, prompt, strlen(prompt));

        int n = 0;
        while(true) {
            int ret = syscall_read(0, cmd_buf, sizeof(cmd_buf));
            if(ret > 0 ) {
                syscall_write(1, cmd_buf + n, ret);
                n += ret;
            }

            if (n > 0 && (cmd_buf[n-1] == '\n' || cmd_buf[n-1] == '\r')) {
                cmd_buf[n-1] = '\0';
                break;
            }
            if(n>=MAX_CMD_LEN) {
                cmd_buf[n-1] = '\0';
                break;
            }
        }

        // 解析命令
        int argc = parse_cmd(cmd_buf, argv);
        if (argc == 0) continue;

        // 执行命令
        if (strcmp(argv[0], "ls") == 0) {
            cmd_ls(argc > 1 ? argv[1] : ".");
        }
        else if (strcmp(argv[0], "cat") == 0) {
            if (argc > 1) cmd_cat(argv[1]);
            else debug_debug("cat: missing operand\n");
        }
        else if (strcmp(argv[0], "rm") == 0) {
            if (argc > 1) cmd_rm(argv[1]);
            else debug_debug("rm: missing operand\n");
        }
        else if (strcmp(argv[0], "exit") == 0) {
            break;
        }
        else {
            // 尝试执行可执行文件
            if (syscall_exec(argv[0], argv) < 0) {
                debug_debug("unknown command: %s\n", argv[0]);
            }
        }
    }

    return 0;
}