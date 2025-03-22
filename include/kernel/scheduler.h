#pragma once
#include <stdint.h>
#include "lib/console.h"
#include "process.h"

// 任务状态段结构
struct TaskStateSegment {
    uint32_t prev_tss;   // 上一个TSS的链接
    uint32_t esp0;       // 特权级0的栈指针
    uint32_t ss0;        // 特权级0的栈段
    uint32_t esp1;       // 特权级1的栈指针
    uint32_t ss1;        // 特权级1的栈段
    uint32_t esp2;       // 特权级2的栈指针
    uint32_t ss2;        // 特权级2的栈段
    uint32_t cr3;        // 页目录基址寄存器
    uint32_t eip;        // 指令指针
    uint32_t eflags;     // 标志寄存器
    uint32_t eax;        // 通用寄存器
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t es;         // 段寄存器
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;
    uint32_t ldt;        // 局部描述符表
    uint16_t trap;       // 调试陷阱标志
    uint16_t iomap_base; // I/O位图基址
};

class Scheduler {
public:
    static void init();
    static void timer_tick();
    static void schedule();
    static TaskStateSegment* get_tss();
    static TaskStateSegment tss;

private:
    static uint32_t current_time_slice;
};