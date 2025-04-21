#pragma once
#include "kernel_memory.h"

#include <cstdint>

#include "kernel/console_device.h"
#include "kernel/list.h"
#include "user_memory.h"

// 进程状态
enum ProcessState {
    PROCESS_NEW,
    PROCESS_READY,
    PROCESS_RUNNING,
    PROCESS_SLEEPING,
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

struct Stacks {
    uint32_t user_stack;      // 用户态栈基址
    uint32_t user_stack_size; // 用户态栈大小

    VADDR kernel_stack;
    uint32_t esp0;       // 内核态栈指针
    uint32_t ss0 = 0x08; // 内核态栈段选择子
    uint32_t ebp0;       // 内核态基址指针
    void print();
    int allocSpace(UserMemory& mm);
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

namespace kernel
{
class FileDescriptor;
}

// 进程控制块结构
#define DEBUG_STATUS_HALT 1 << 0
#define DEFAULT_TIME_SLICE 100
struct Context;
struct Task {
    uint32_t task_id;            // 进程ID
    char name[PROCNAME_LEN + 1]; // 进程名称

    ProcessState state;          // 进程状态
    uint32_t priority;           // 进程优先级
    int32_t time_slice = DEFAULT_TIME_SLICE;         // 时间片
    uint32_t total_time;         // 总执行时间
    uint32_t exit_status;        // 退出状态码
    uint32_t sleep_ticks;
    struct kernel::list_head sched_list; // 调度链表节点
    uint32_t affinity = 0;               // CPU亲和性掩码

    Registers regs;
    Stacks stacks;
    Task* next = nullptr;
    Task* prev = nullptr;
    Context* context = nullptr;
    volatile uint32_t debug_status = 0;
    void print();
    void alloc_stack(KernelMemory& mm);
    int allocUserStack();

    int cpu = -1;
};
struct Context {
    uint32_t context_id;

    UserMemory user_mm; // 用户空间内存管理器

    kernel::FileDescriptor* fd_table[MAX_PROCESS_FDS] = {nullptr};
    int next_fd = 3;     // 0, 1, 2 保留给标准输入、输出和错误

    char cwd[256] = "/"; // 当前工作目录，默认为根目录

    int allocate_fd();
    void print();
    void cloneMemorySpace(Context *source);
    void cloneFiles(Context *source);
};


class PidManager
{
public:
    static int32_t alloc();
    static void free(uint32_t pid);
    static void initialize();

private:
    static constexpr uint32_t MAX_PID = 32768;
    static uint32_t pid_bitmap[(MAX_PID + 31) / 32];
};

class ProcessManager
{
public:
    static void init();
    static Context* create_context(const char* name); // 修改内部实现使用PidManager
    static Task* kernel_task(
        Context* ctx, const char* name, uint32_t entry, uint32_t argc, char* argv[]);
    static int fork();
    static Task* get_current_task();
    // static int32_t execute_process(const char* path);
    static bool schedule(); // false for no more processes
    // static int switch_process(uint32_t pid);
    static PidManager pid_manager;
    static PidManager tid_manager;
    static void switch_to_user_mode(uint32_t entry_point, Task* task);
    static void save_context(uint32_t int_num, uint32_t* esp);
    static void restore_context(uint32_t int_num, uint32_t* esp);

    // static void cloneMemory(ProcessControlBlock* pcb);
    static void sleep_current_process(uint32_t ticks);
    static Context* kernel_context;
    static struct Debug
    {
        bool is_task_switch = false;
        Task * prev_task = nullptr;
        Task * cur_task = nullptr;
    } debug;

private:
    static kernel::ConsoleFS console_fs;
};