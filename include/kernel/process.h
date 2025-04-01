#pragma once
#include "lib/console.h"
#include <kernel/vfs.h>
#include <stdint.h>

#include "kernel/console_device.h"
#include "user_memory.h"

// 进程状态
enum ProcessState {
    PROCESS_NEW,
    PROCESS_READY,
    PROCESS_RUNNING,
    PROCESS_WAITING,
    PROCESS_TERMINATED,
    EXITED
};

#define KERNEL_STACK_SIZE 1024 * 16
#define USER_STACK_SIZE 1024 * 4096
#define KERNEL_CS 0x08
#define KERNEL_DS 0x10
#define USER_CS 0x1B
#define USER_DS 0x23
#define PROCNAME_LEN 31
#define MAX_PROCESS_FDS 256

struct Stacks
{
    uint32_t user_stack;      // 用户态栈基址
    uint32_t user_stack_size; // 用户态栈大小

    uint32_t esp0;              // 内核态栈指针
    uint32_t ss0 = 0x08;        // 内核态栈段选择子
    uint32_t ebp0;              // 内核态基址指针
    void print();
    int allocSpace(UserMemory &mm);
};
struct Registers {
    // 8个通用寄存器
    uint32_t eax; // 通用寄存器
    uint32_t ebx; // 通用寄存器
    uint32_t ecx; // 通用寄存器
    uint32_t edx; // 通用寄存器
    uint32_t esi; // 源变址寄存器
    uint32_t edi; // 目的变址寄存器
    uint32_t esp; // 用户态栈指针
    uint32_t ebp; // 用户态基址指针

    // 6个段寄存器
    uint16_t cs; // 代码段选择子
    uint16_t ds; // 数据段选择子
    uint16_t ss; // 栈段选择子
    uint16_t es; // 附加段选择子
    uint16_t fs; // 附加段选择子
    uint16_t gs; // 附加段选择子

    // 四个控制寄存器
    // cr0, cr1, cr2, cr3
    uint32_t cr3; // 页目录基址

    // 两个额外寄存器
    uint32_t eip;    // 指令指针
    uint32_t eflags; // 标志寄存器
    void print();
};

// 进程控制块结构
struct ProcessControlBlock {
    ProcessControlBlock* next = nullptr;
    ProcessControlBlock* prev = nullptr;
    uint32_t pid;       // 进程ID
    ProcessState state; // 进程状态
    char name[PROCNAME_LEN + 1];              // 进程名称
    uint32_t priority;                        // 进程优先级
    uint32_t time_slice;                      // 时间片
    uint32_t total_time;                      // 总执行时间
    uint32_t exit_status; // 退出状态码

    UserMemory user_mm;        // 用户空间内存管理器
    Registers regs;
    Stacks stacks;

    kernel::FileDescriptor* fd_table[MAX_PROCESS_FDS] = {nullptr};
    inline int allocate_fd()
    {
        return next_fd++;
    }
    volatile int next_fd = 3;      // 0, 1, 2 保留给标准输入、输出和错误
    void print();
};
union PCB
{
    struct ProcessControlBlock pcb;
    uint8_t stack[KERNEL_STACK_SIZE];
};

// 修改后的当前进程获取宏
#define CURRENT() ({ \
    uint32_t esp; \
    asm volatile("mov %%esp, %0" : "=r"(esp)); \
    (PCB*)(esp & ~0x3FFF); \
})



class PidManager {
public:
    static int32_t alloc();
    static void free(uint32_t pid);
    static void initialize();

private:
    static constexpr uint32_t MAX_PID = 32768;
    static uint32_t pid_bitmap[(MAX_PID + 31) / 32];
};



class ProcessManager {
public:
    static void init();
    static ProcessControlBlock* create_process(const char* name); // 修改内部实现使用PidManager
    static ProcessControlBlock* kernel_process(const char* name, uint32_t entry, uint32_t argc, char* argv[]);
    static int fork();
    static ProcessControlBlock* get_current_process();
    // static int32_t execute_process(const char* path);
    static bool schedule(); // false for no more processes
    // static int switch_process(uint32_t pid);
    static PidManager pid_manager;
    static void switch_to_user_mode(uint32_t entry_point, ProcessControlBlock*pcb);
    static void save_context(uint32_t* esp);
    static void restore_context(uint32_t* esp);
    static void initIdle();
    static void appendPCB(PCB* pcb);
    static int allocUserStack(ProcessControlBlock* pcb);
    static ProcessControlBlock * current_pcb;

private:
    static PCB * idle_pcb;
    static kernel::ConsoleFS console_fs;
};