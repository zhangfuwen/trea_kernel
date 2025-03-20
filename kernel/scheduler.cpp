#include "scheduler.h"

// 初始化调度器
void Scheduler::init() {
    current_time_slice = 0;
    // 初始化TSS
    tss = {};
    tss.ss0 = 0x10;  // 内核数据段
    tss.esp0 = 0;    // 将在进程切换时设置
    Console::print("Scheduler initialized\n");
}

// 时钟中断处理
void Scheduler::timer_tick() {
    current_time_slice++;
    if (current_time_slice >= 100) { // 时间片到期
        current_time_slice = 0;
        schedule();
    }
}

// 进程调度
void Scheduler::schedule() {
    ProcessControlBlock* current = ProcessManager::get_current_process();
    if (current) {
        // 保存当前进程上下文
        current->esp = tss.esp;
        current->ebp = tss.ebp;
        current->eip = tss.eip;
    }

    // 调用进程管理器进行进程切换
    ProcessManager::schedule();

    // 加载新进程的上下文
    ProcessControlBlock* next = ProcessManager::get_current_process();
    if (next) {
        tss.esp = next->esp;
        tss.ebp = next->ebp;
        tss.eip = next->eip;
        tss.cr3 = next->cr3;
    }
}

// 获取当前TSS
TaskStateSegment* Scheduler::get_tss() {
    return &tss;
}

// 静态成员初始化
TaskStateSegment Scheduler::tss;
uint32_t Scheduler::current_time_slice = 0;