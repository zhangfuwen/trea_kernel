#include "kernel/process.h"
#include "kernel/buddy_allocator.h"
#include <kernel/vfs.h>
#include <lib/console.h>
#include <kernel/console_device.h>

#include "kernel/process.h"
#include <cstdint>
#include <kernel/kernel.h>
#include <lib/debug.h>
#include "arch/x86/gdt.h"



void ProcessManager::init() {
    next_pid = 1;
    current_pid = 0;
    Console::print("ProcessManager initialized\n");
}

kernel::ConsoleFS console_fs;
// 创建新进程
uint32_t ProcessManager::create_process(const char* name) {
    uint32_t pid = next_pid++;
    if (pid >= MAX_PROCESSES) {
        debug_debug("ProcessManager: Maximum number of processes reached\n");
        return 0; // 进程数达到上限
    }

    ProcessControlBlock& pcb = processes[pid];
    auto pgd = Kernel::instance().kernel_mm().paging().getCurrentPageDirectory();
    debug_debug("ProcessManager: Current Page Directory: %x\n", pgd);
    pcb.mm.init(
        (uint32_t)pgd,
        []() {
            auto page = Kernel::instance().kernel_mm().allocPage();
            debug_debug("ProcessManager: Allocated Page at %x\n", page);
            return page;

        },
        [](uint32_t physAddr) {
            Kernel::instance().kernel_mm().freePage(physAddr);
        },
        [](uint32_t physAddr) {
            return (void*)Kernel::instance().kernel_mm().getVirtualAddress(physAddr);
        }
        );
    pcb.pid = pid;
    pcb.state = PROCESS_NEW;
    pcb.priority = 1;
    pcb.time_slice = 100;
    pcb.total_time = 0;

    // 分配用户态栈（4MB）
    pcb.user_stack = (uint32_t)pcb.mm.allocate_area(4*1024*1024, PAGE_WRITE, 0);
    debug_debug("ProcessManager: Allocated user stack: %x\n", pcb.user_stack);
    if (!pcb.user_stack) {
        debug_debug("ProcessManager: Failed to allocate user stack\n");
        return 0;
    }
    pcb.esp = pcb.user_stack + 4 * 1024 * 1024 - 16;

    // 分配内核态栈（4MB）
    pcb.kernel_stack = (uint32_t)Kernel::instance().kernel_mm().kmalloc(1024*4096);
    if (!pcb.kernel_stack) {
        debug_debug("ProcessManager: Failed to allocate kernel stack\n");
        Kernel::instance().kernel_mm().kfree((void*)pcb.user_stack);
        return 0;
    }
    pcb.esp0 = pcb.kernel_stack + 1024 * 1024 - 16;

    // 设置段选择子
    pcb.cs = 0x1B; // 用户代码段选择子（RPL=3）
    pcb.ds = 0x23; // 用户数据段选择子（RPL=3）
    pcb.ss = 0x23; // 用户栈段选择子（RPL=3）

    // 复制进程名称
    for (int i = 0; i < 31 && name[i]; i++) {
        pcb.name[i] = name[i];
    }
    pcb.name[31] = '\0';

    // 初始化标准输入输出流
    pcb.stdin = console_fs.open("/dev/console");
    pcb.stdout = console_fs.open("/dev/console");
    pcb.stderr = console_fs.open("/dev/console");


    debug_debug("Created process id: %d\n", pid);
    return pid;
}
kernel::ConsoleFS ProcessManager::console_fs;

void ProcessControlBlock::print() {
    debug_debug("Process: %s\n", name);
    debug_debug("  PID: %d\n", pid);
    debug_debug("  State: %d\n", state);
    debug_debug("  EIP: %d\n", eip);
    debug_debug("  EBP: %d\n", ebp);
    debug_debug("  ESP: %d\n", esp);
    debug_debug("  EBP: %d\n", ebp);
    debug_debug("  EIP: %d\n", eip);
    debug_debug("  SS: %d\n", ss);
    debug_debug("  EIP: %d\n", eip);
    debug_debug("  CR3: %d\n", cr3);
    debug_debug("  Priority: %d\n", priority);
    debug_debug("  TimeSlice: %d\n", time_slice);
    debug_debug("  TotalTime: %d\n", total_time);
    debug_debug("  UserStack: %d\n", user_stack);
    debug_debug("  KernelStack: %d\n", kernel_stack);
    debug_debug("  EFLAGS: %d\n", eflags);
    debug_debug("  EAX: %d\n", eax);
    debug_debug("  EBX: %d\n", ebx);
    debug_debug("  ECX: %d\n", ecx);
    debug_debug("  EDX: %d\n", edx);
    debug_debug("  ESI: %d\n", esi);
    debug_debug("  EDI: %d\n", edi);
    debug_debug("  EBP: %d\n", ebp);
    debug_debug("  ESP: %d\n", esp);

}
// fork系统调用实现
int ProcessManager::fork() {
    debug_debug("fork enter!\n");
    ProcessControlBlock* parent = &processes[current_pid];
    uint32_t child_pid = create_process(parent->name);
    if (child_pid == 0) {
        debug_err("Create process failed\n");
        return -1;
    }
    parent->print();

    ProcessControlBlock& child = processes[child_pid];

    // 复制父进程的寄存器状态
    child.esp = parent->esp;
    child.ebp = parent->ebp;
    child.eip = parent->eip;
    child.eflags = parent->eflags;
    child.eax = 0;  // 子进程返回0
    child.ebx = parent->ebx;
    child.ecx = parent->ecx;
    child.edx = parent->edx;
    child.esi = parent->esi;
    child.edi = parent->edi;

    // 复制父进程的优先级和时间片
    child.priority = parent->priority;
    child.time_slice = parent->time_slice;

    // 复制内存空间
    Kernel& kernel = Kernel::instance();

    PageDirectory * dst;
    PageDirectory * src = (PageDirectory*)Kernel::instance().kernel_mm().getVirtualAddress(parent->cr3);
    int ret = kernel.kernel_mm().paging().copyMemorySpace(src, dst);
    if (ret < 0) {
        debug_err("Failed to copy memory space for child process %d\n", child_pid);
        // 如果内存复制失败，清理并返回错误
        child.state = PROCESS_TERMINATED;
        kernel.kernel_mm().kfree((void*)child.user_stack);
        kernel.kernel_mm().kfree((void*)child.kernel_stack);
        return -1;
    }
    child.cr3 = (uint32_t)dst;

    // 复制文件描述符
    child.stdin = parent->stdin;
    child.stdout = parent->stdout;
    child.stderr = parent->stderr;

    // 设置子进程状态为就绪
    child.state = PROCESS_READY;

    debug_debug("fork return, parent: %d, child:%d\n", parent->pid, child_pid);
    // 父进程返回子进程PID
    return child_pid;
}

// 获取当前运行的进程
ProcessControlBlock* ProcessManager::get_current_process() {
    return &processes[current_pid];
}

// 切换到下一个进程
bool ProcessManager::schedule() {
    // 简单的轮转调度

    uint32_t next = (current_pid + 1) % MAX_PROCESSES;
    while (next != current_pid) {
        //debug_debug("schedule called, current: %d, next:%d, nextstate:%d\n", current_pid, next, processes[next].state);
        if (processes[next].state == PROCESS_READY) {
            processes[next].state = PROCESS_RUNNING;
            break;
        }
        next = (next + 1) % MAX_PROCESSES;
    }
    if (next == current_pid) {
        debug_debug("equal process: %d\n", next);
        return false; // 所有进程都处于就绪状态，无需切换
    } else {
        processes[current_pid].state = PROCESS_READY;
        debug_debug("switch to next process: %d\n", next);
        current_pid = next;
    }
    return true;
}

// 进程切换函数，切换到指定进程
int ProcessManager::switch_process(uint32_t pid) {
    debug_debug("Switching process from %d to %d\n", current_pid, pid);
    debug_debug("state: current(%d), next(%d)\n", processes[current_pid].state, processes[pid].state);
    if (pid >= MAX_PROCESSES || processes[pid].state != PROCESS_NEW) {
        debug_debug("invalid pid or process not ready\n");
        return current_pid; // 无效的进程ID或进程不处于就绪状态
    }

    // // 将当前进程状态设为就绪
    // if (current_pid != 0) {
    //     ProcessControlBlock* current = &processes[current_pid];
    //     current->state = PROCESS_READY;
    // }

    // 切换到新进程
    // current_pid = pid;

    // // 更新TSS中的内核栈指针和段选择子
    // GDT::updateTSS(next->esp0, next->ss0);
    //
    // // 更新TSS中的页目录基址
    // GDT::updateTSSCR3(next->cr3);

    if (current_pid == pid) {
        return -1;
    }
    ProcessControlBlock* next = &processes[pid];

    // 切换页目录
    // asm volatile("mov %0, %%cr3" : : "r"(next->cr3));
    current_pid = pid;
    processes[current_pid].state = PROCESS_RUNNING;

    // 返回新进程的ID
    return current_pid;
}

// 保存当前进程的寄存器状态（由中断处理程序调用）
void ProcessManager::save_context(uint32_t* esp) {
    // if (current_pid == 0) return;
    
    ProcessControlBlock* current = &processes[current_pid];
    // 从中断栈中获取寄存器状态
    current->eax = esp[7];  // pushad的顺序：eax,ecx,edx,ebx,esp,ebp,esi,edi
    current->ecx = esp[6];
    current->edx = esp[5];
    current->ebx = esp[4];
    current->esp = esp[3];
    current->ebp = esp[2];
    current->esi = esp[1];
    current->edi = esp[0];
    
    // 获取eflags和eip（从中断栈中）
    current->eflags = esp[14];  // 中断栈：gs,fs,es,ds,pushad,vector,error,eip,cs,eflags
    current->eip = esp[12];
    
    // 保存段寄存器
    current->ds = esp[8];
    current->es = esp[9];
    current->fs = esp[10];
    current->gs = esp[11];
}

// 恢复新进程的寄存器状态（由中断处理程序调用）
void ProcessManager::restore_context(uint32_t* esp) {
    ProcessControlBlock* next = &processes[current_pid];
    
    // 恢复通用寄存器
    esp[7] = next->eax;
    esp[6] = next->ecx;
    esp[5] = next->edx;
    esp[4] = next->ebx;
    esp[3] = next->esp;
    esp[2] = next->ebp;
    esp[1] = next->esi;
    esp[0] = next->edi;
    
    // 恢复eflags和eip
    esp[14] = next->eflags;
    esp[12] = next->eip;
    
    // 恢复段寄存器
    esp[8] = next->ds;
    esp[9] = next->es;
    esp[10] = next->fs;
    esp[11] = next->gs;
}
void ProcessManager::switch_to_user_mode(uint32_t entry_point) {
    auto user_stack = get_current_process()->user_stack;
    debug_debug("switch_to_user_mode called, user stack: %x, entry point %x\n", user_stack, entry_point);
    /*
    asm volatile(
        "mov %%eax, %%esp\n\t"   // 设置用户栈指针
        "push $0x23\n\t"        // 用户数据段选择子
        "push %%eax\n\t"        // 用户栈指针
        "push $0x200\n\t"       // EFLAGS
        "push $0x1B\n\t"        // 用户代码段选择子
        "push %%ebx\n\t"        // 入口地址
        "lcall $0x33, $0\n\t"   // 调用门切换
        :
        : "a" (user_stack), "b" (entry_point)
        : "memory"
    );
    */
    // GDT::updateTSS(get_current_process()->esp0, get_current_process()->ss0);
    // GDT::updateTSSCR3(get_current_process()->cr3);

    asm volatile(
      "mov %%eax, %%esp\n\t"   // 设置用户栈指针
      "pushl $0x23\n\t"        // 用户数据段选择子
      "pushl %%eax\n\t"        // 用户栈指针
      "pushfl\n\t"             // 压入当前的 EFLAGS
      "orl $0x200, (%%esp)\n\t" // 设置 IF 标志位
      "pushl $0x1B\n\t"        // 用户代码段选择子
      "pushl %%ebx\n\t"        // 入口地址
      "iret\n\t"               // 进行特权级切换到用户态
      :
      : "a" (user_stack), "b" (entry_point)
      : "memory", "cc"
    );
    debug_debug("switch_to_user_mode completed\n");
}

/*
// 切换到用户模式函数
void ProcessManager::switch_to_user_mode(uint32_t entry_point, uint32_t user_stack) {
    // 准备用户态栈
    uint32_t esp = user_stack;
    
    // 设置用户态段选择子
    uint16_t user_cs = 0x1B; // 用户代码段选择子（RPL=3）
    uint32_t user_ds = 0x23; // 用户数据段选择子（RPL=3）
    
    // 使用iret指令切换到用户态
    // 需要在栈上按顺序准备：ss, esp, eflags, cs, eip
    asm volatile(
        "cli\n"                  // 关中断
        "mov %0, %%eax\n"        // 用户数据段选择子
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
        : "r"(user_ds), "r"(esp), "r"(user_cs), "r"(entry_point)
        : "eax"
    );
}
*/

// 静态成员初始化
ProcessControlBlock ProcessManager::processes[ProcessManager::MAX_PROCESSES];
uint32_t ProcessManager::next_pid = 1;
uint32_t ProcessManager::current_pid = 0;