#include "kernel/scheduler.h"

#include <lib/debug.h>

// 初始化调度器
void Scheduler::init()
{
    current_time_slice = 0;
    // 初始化TSS
    tss = {};
    tss.ss0 = 0x10; // 内核数据段
    tss.esp0 = 0;   // 将在进程切换时设置
    Console::print("Scheduler initialized\n");
}

// 时钟中断处理
void Scheduler::timer_tick()
{
    Task* current = ProcessManager::get_current_task();
    if(current && current->state == ProcessState::PROCESS_RUNNING) {
        current_time_slice++;
        // debug_debug("Timer tick: PID=%d, time_slice=%d\n", current->pid,
        // current_time_slice);
        if(current_time_slice >= 100) { // 时间片到期
            debug_debug("Time slice expired for PID=%d\n", current->task_id);
            current_time_slice = 0;
            ProcessManager::schedule();
        }
    } else {
        debug_debug("No running process, trying to schedule\n");
        // 如果当前没有运行的进程，尝试调度新进程
        ProcessManager::schedule();
    }
}

// 进程调度
void Scheduler::schedule()
{
    // ProcessControlBlock* current1 = ProcessManager::get_current_process();
    // if(current1 && current1->state == ProcessState::PROCESS_RUNNING) {
    //     debug_debug("[Schedule] Current process PID=%d, State=RUNNING -> READY\n", current1->pid);
    //     debug_debug("[Schedule] Saving context - ESP=0x%x, EBP=0x%x, EIP=0x%x\n", current1->esp,
    //         current1->ebp, current1->eip);
    //     // 保存当前进程上下文
    //     // 如果是从用户态切换来的，CPU已经自动保存了SS、ESP、EFLAGS、CS和EIP
    //     // 如果是从内核态切换来的，只需要保存通用寄存器
    //     asm volatile("pushf\n"         // 保存标志寄存器
    //                  "push %%cs\n"     // 保存代码段
    //                  "push $.L1\n"     // 保存返回地址
    //                  "mov %%esp, %0\n" // 保存栈指针
    //                  "mov %%ebp, %1\n" // 保存基址指针
    //                  ".L1:\n"          // 定义局部标签
    //                  : "=m"(current1->esp), "=m"(current1->ebp)
    //                  :
    //                  : "memory");
    //     current1->state = ProcessState::PROCESS_READY;
    //     debug_debug("[Schedule] Context saved for PID=%d\n", current1->pid);
    // }
    //
    // // 调用进程管理器进行进程切换
    // debug_debug("[Schedule] Calling process manager to switch process\n");
    // bool hasNew = ProcessManager::schedule();
    // if(!hasNew) {
    //     debug_debug("[Schedule] No new process to schedule\n");
    //     return;
    // }
    //
    // // 加载新进程的上下文
    // ProcessControlBlock* next = ProcessManager::get_current_process();
    // if(next && next->state != ProcessState::EXITED) {
    //     debug_debug(
    //         "[Schedule] Next process PID=%d, State=%d -> RUNNING\n", next->pid, next->state);
    //     debug_debug("[Schedule] Loading context - ESP=0x%x, EBP=0x%x, EIP=0x%x\n", next->esp,
    //         next->ebp, next->eip);
    //     debug_debug("[Schedule] Switching page directory to CR3=0x%x\n", next->cr3);
    //     debug_debug("[Schedule] Setting kernel stack to ESP0=0x%x\n", next->kernel_stack);
    //
    //     next->state = ProcessState::PROCESS_RUNNING;
    //     tss.esp0 = next->kernel_stack; // 设置内核栈
    //     tss.cr3 = next->cr3;           // 切换页目录
    //
    //     // 切换到新进程
    //     asm volatile("mov %0, %%cr3\n"
    //                  "mov %1, %%esp\n"
    //                  "mov %2, %%ebp\n"
    //                  "jmp *%3\n"
    //                  :
    //                  : "r"(next->cr3), "r"(next->esp), "r"(next->ebp), "r"(next->eip));
    // } else {
    //     debug_debug("[Schedule] No process to run, halting\n");
    // }
}

// 获取当前TSS
TaskStateSegment* Scheduler::get_tss()
{
    return &tss;
}

// 静态成员初始化
TaskStateSegment Scheduler::tss;
uint32_t Scheduler::current_time_slice = 0;