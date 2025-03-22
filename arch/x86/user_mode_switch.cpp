#include <cstdint>
#include "arch/x86/gdt.h"
#include "lib/debug.h"

// 用户态切换处理函数，作为调用门的目标
extern "C" void user_mode_entry();

// 用户程序入口点和栈指针
static uint32_t user_entry_point = 0;
static uint32_t user_stack_pointer = 0;

// 设置用户程序入口点和栈指针
extern "C" void set_user_entry(uint32_t entry, uint32_t stack) {
    user_entry_point = entry;
    user_stack_pointer = stack;
}

// 用户态切换处理函数的C++部分
// 内核栈保存区域
static uint32_t kernel_esp = 0;

extern "C" void user_mode_switch_handler() {
    // 保存内核栈指针
    asm volatile("mov %%esp, %0" : "=m"(kernel_esp));

    // 设置用户态段选择子
    uint16_t user_cs = 0x1B; // 用户代码段选择子（RPL=3）
    uint16_t user_ds = 0x23; // 用户数据段选择子（RPL=3）

    // 使用iret指令切换到用户态
    asm volatile(
        "cli\n"
        "mov %0, %%ax\n"
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"
        
        "push %0\n"      // 用户栈段选择子(ss)
        "push %1\n"      // 用户栈指针(esp)
        "pushf\n"        // eflags
        "pop %%eax\n"
        "or $0x200, %%eax\n"  // 启用中断
        "push %%eax\n"
        "push %2\n"      // 用户代码段选择子(cs)
        "push %3\n"      // 用户入口点(eip)
        "iret\n"
        : 
        : "r"(user_ds), "r"(user_stack_pointer), "r"(user_cs), "r"(user_entry_point)
        : "eax", "memory"
    );
}

// 系统调用返回处理函数
__attribute__((naked)) 
extern "C" void syscall_entry() {
    asm volatile(
        "pusha\n"
        "mov %0, %%esp\n"  // 恢复内核栈
        "sti\n"
        "ret\n"
        : 
        : "m"(kernel_esp)
    );
}