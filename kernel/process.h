#pragma once
#include <stdint.h>
#include "lib/console.h"
#include <kernel/vfs.h>
#include "kernel/console_device.h"

// 进程状态
enum ProcessState {
    PROCESS_NEW,
    PROCESS_READY,
    PROCESS_RUNNING,
    PROCESS_WAITING,
    PROCESS_TERMINATED,
    EXITED
};

// 进程控制块结构
struct ProcessControlBlock {
    uint32_t pid;                  // 进程ID
    ProcessState state;            // 进程状态
    uint32_t esp;                  // 用户态栈指针
    uint32_t ebp;                  // 用户态基址指针
    uint32_t eip;                  // 指令指针
    uint32_t esp0;                 // 内核态栈指针
    uint32_t ebp0;                 // 内核态基址指针
    uint32_t cr3;                  // 页目录基址
    uint32_t priority;             // 进程优先级
    uint32_t time_slice;           // 时间片
    uint32_t total_time;           // 总执行时间
    uint32_t user_stack;           // 用户态栈基址
    uint32_t kernel_stack;         // 内核态栈基址
    uint16_t cs;                   // 代码段选择子
    uint16_t ds;                   // 数据段选择子
    uint16_t ss;                   // 栈段选择子
    char name[32];                 // 进程名称
    kernel::FileDescriptor* stdin;  // 标准输入
    kernel::FileDescriptor* stdout; // 标准输出
    kernel::FileDescriptor* stderr; // 标准错误
    uint32_t exit_status;          // 退出状态码
};

class ProcessManager {
public:
    static void init();
    static uint32_t create_process(const char* name);
    static int fork();
    static ProcessControlBlock* get_current_process();
    static void schedule();
    static void switch_process(uint32_t pid);
    static void switch_to_user_mode(uint32_t entry_point, uint32_t user_stack);

private:
    static uint32_t create_page_directory();
    static void copy_page_tables(uint32_t src_cr3, uint32_t dst_cr3);
    static const uint32_t MAX_PROCESSES = 64;
    static ProcessControlBlock processes[MAX_PROCESSES];
    static uint32_t next_pid;
    static uint32_t current_process;
    static bool copy_memory_space(ProcessControlBlock& parent, ProcessControlBlock& child);
    static kernel::ConsoleFS console_fs;
};