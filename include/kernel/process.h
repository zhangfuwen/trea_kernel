#pragma once
#include <stdint.h>
#include "lib/console.h"
#include <kernel/vfs.h>

#include "kernel/console_device.h"

// 内存区域描述符
struct MemoryArea {
    uint32_t start_addr;     // 起始地址
    uint32_t end_addr;       // 结束地址
    uint32_t flags;          // 访问权限标志
    uint32_t type;           // 区域类型(代码段、数据段、堆、栈等)
};

// 进程虚拟地址空间管理器
class UserMemory {
public:
    // 初始化内存管理器
    void init(uint32_t page_dir);
    
    // 分配一个新的内存区域
    bool allocate_area(uint32_t start, uint32_t size, uint32_t flags, uint32_t type);
    
    // 释放指定地址范围的内存区域
    void free_area(uint32_t start, uint32_t size);
    
    // 扩展或收缩堆区
    uint32_t brk(uint32_t new_brk);
    
    // 映射物理页面到虚拟地址空间
    bool map_pages(uint32_t virt_addr, uint32_t phys_addr, uint32_t size, uint32_t flags);
    
    // 解除虚拟地址空间的映射
    void unmap_pages(uint32_t virt_addr, uint32_t size);

private:
    static const uint32_t MAX_MEMORY_AREAS = 32;  // 最大内存区域数

    uint32_t pgd;                    // 页目录基地址
    uint32_t start_code;             // 代码段起始地址
    uint32_t end_code;               // 代码段结束地址
    uint32_t start_data;             // 数据段起始地址
    uint32_t end_data;               // 数据段结束地址
    uint32_t start_heap;             // 堆区起始地址
    uint32_t end_heap;               // 堆区当前结束地址
    uint32_t start_stack;            // 栈区起始地址
    uint32_t end_stack;              // 栈区结束地址
    uint32_t total_vm;               // 总虚拟内存大小(页数)
    uint32_t locked_vm;              // 锁定的虚拟内存大小(页数)
    MemoryArea areas[MAX_MEMORY_AREAS]; // 内存区域数组
    uint32_t num_areas;              // 当前内存区域数量
};

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
    uint32_t ss0;                  // 内核态栈段选择子
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
    uint16_t es;                   // 附加段选择子
    uint16_t fs;                   // 附加段选择子
    uint16_t gs;                   // 附加段选择子
    uint32_t eflags;               // 标志寄存器
    uint32_t eax;                  // 通用寄存器
    uint32_t ebx;                  // 通用寄存器
    uint32_t ecx;                  // 通用寄存器
    uint32_t edx;                  // 通用寄存器
    uint32_t esi;                  // 源变址寄存器
    uint32_t edi;                  // 目的变址寄存器
    char name[32];                 // 进程名称
    kernel::FileDescriptor* stdin;  // 标准输入
    kernel::FileDescriptor* stdout; // 标准输出
    kernel::FileDescriptor* stderr; // 标准错误
    uint32_t exit_status;          // 退出状态码
    UserMemory mm;              // 用户空间内存管理器
    void print();
};

class ProcessManager {
public:
    static void init();
    static uint32_t create_process(const char* name);
    static int fork();
    static ProcessControlBlock* get_current_process();
    static int32_t execute_process(const char* path);
    static bool schedule(); // false for no more processes
    static int switch_process(uint32_t pid);
    static void switch_to_user_mode(uint32_t entry_point, uint32_t user_stack);
    static void save_context(uint32_t* esp);
    static void restore_context(uint32_t* esp);

private:
    static uint32_t current_pid;
    static const uint32_t MAX_PROCESSES = 64;
    static ProcessControlBlock processes[MAX_PROCESSES];
    static uint32_t next_pid;
    static kernel::ConsoleFS console_fs;
};