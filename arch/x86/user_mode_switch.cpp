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
extern "C" void user_mode_switch_handler() {
    // 设置用户态段选择子
    uint16_t user_cs = 0x1B; // 用户代码段选择子（RPL=3）
    uint16_t user_ds = 0x23; // 用户数据段选择子（RPL=3）
    
    // 使用iret指令切换到用户态
    // 需要在栈上按顺序准备：ss, esp, eflags, cs, eip
    asm volatile(
        "cli\n"                  // 关中断
        "mov %0, %%ax\n"        // 用户数据段选择子
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"
        
        "push %0\n"              // 用户栈段选择子(ss)
        "push %1\n"              // 用户栈指针(esp)
        "pushf\n"                // 标志寄存器(eflags)
        "pop %%eax\n"            // 获取当前eflags
        "or $0x200, %%eax\n"     // 设置IF位（启用中断）
        "push %%eax\n"           // 压入修改后的eflags
        "push %2\n"              // 用户代码段选择子(cs)
        "push %3\n"              // 用户代码入口点(eip)
        "iret\n"                 // 特权级转换并跳转到用户代码
        : 
        : "r"(user_ds), "r"(user_stack_pointer), "r"(user_cs), "r"(user_entry_point)
        : "eax"
    );
}