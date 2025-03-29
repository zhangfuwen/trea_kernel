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
// 创建新进程框架
uint32_t ProcessManager::create_process(const char* name) {
    uint32_t pid = next_pid++;
    if (pid >= MAX_PROCESSES) {
        debug_debug("ProcessManager: Maximum number of processes reached\n");
        return 0;
    }

    ProcessControlBlock& pcb = processes[pid];
    pcb.pid = pid;
    pcb.state = PROCESS_NEW;

    // 初始化进程基础结构
    if (!init_process(pid, name)) {
        next_pid--; // 回滚PID分配
        processes[pid] = ProcessControlBlock(); // 重置PCB
        return 0;
    }

    return pid;
}

// 初始化进程详细信息
bool ProcessManager::init_process(uint32_t pid, const char* name) {
    auto pcb = processes[pid];
    auto pgd = Kernel::instance().kernel_mm().paging().getCurrentPageDirectory();
    debug_debug("ProcessManager: Current Page Directory: %x\n", pgd);

    // 初始化内存管理
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
    pcb.state = PROCESS_NEW;
    pcb.priority = 1;
    pcb.time_slice = 100;
    pcb.total_time = 0;

    // 分配用户态栈
    pcb.user_stack_size = USER_STACK_SIZE;
    pcb.user_stack = (uint32_t)pcb.mm.allocate_area(pcb.user_stack_size, PAGE_WRITE, MEM_TYPE_STACK);
    if (!pcb.user_stack) {
        debug_debug("ProcessManager: Failed to allocate user stack\n");
        return false;
    }

    // 分配内核态栈
    pcb.kernel_stack_size = KERNEL_STACK_SIZE;
    pcb.kernel_stack = (uint32_t)Kernel::instance().kernel_mm().kmalloc(pcb.kernel_stack_size);
    if (!pcb.kernel_stack) {
        debug_debug("ProcessManager: Failed to allocate kernel stack\n");
        Kernel::instance().kernel_mm().kfree((void*)pcb.user_stack);
        return false;
    }

    pcb.esp = pcb.user_stack + pcb.user_stack_size - 16;
    pcb.ss0 = KERNEL_DS;
    pcb.esp0 = pcb.kernel_stack + pcb.kernel_stack_size - 16;

    // 设置段选择子
    pcb.cs = KERNEL_CS;
    pcb.ds = KERNEL_DS;
    pcb.ss = KERNEL_DS;
    pcb.es = KERNEL_DS;
    pcb.fs = KERNEL_DS;
    pcb.gs = KERNEL_DS;

    // 复制进程名称
    for (int i = 0; i < PROCNAME_LEN && name[i]; i++) {
        pcb.name[i] = name[i];
    }
    pcb.name[PROCNAME_LEN] = '\0';

    // 初始化标准输入输出流
    pcb.fd_table[0] = console_fs.open("/dev/console");
    pcb.fd_table[1] = console_fs.open("/dev/console");
    pcb.fd_table[2] = console_fs.open("/dev/console");


    debug_debug("initialized process id: %d\n", pcb.pid);
    pcb.print();
    return true;
}
kernel::ConsoleFS ProcessManager::console_fs;

void ProcessControlBlock::print() {
    debug_debug("Process: %s (PID: %d)\n", name, pid);
    debug_debug("  State: %d, Priority: %d, Time: %d/%d\n",
              state, priority, total_time, time_slice);

    // 寄存器状态
    debug_debug("  Registers:\n");
    debug_debug("    EAX: 0x%x  EBX: 0x%x  ECX: 0x%x  EDX: 0x%x\n",
              eax, ebx, ecx, edx);
    debug_debug("    ESI: 0x%x  EDI: 0x%x  EBP: 0x%x  ESP: 0x%x\n",
              esi, edi, ebp, esp);
    debug_debug("    EIP: 0x%x  EFLAGS: 0x%x\n", eip, eflags);

    // 内核态信息
    debug_debug("  Kernel State:\n");
    debug_debug("    CR3: 0x%x  ESP0: 0x%x  EBP0: 0x%x\n",
              cr3, esp0, ebp0);

    // 段寄存器
    debug_debug("  Segment Selectors:\n");
    debug_debug("    CS: 0x%x  DS: 0x%x  SS: 0x%x\n", cs, ds, ss);
    debug_debug("    ES: 0x%x  FS: 0x%x  GS: 0x%x\n", es, fs, gs);

    // 内存布局
    debug_debug("  Memory Layout:\n");
    debug_debug("    User Stack:  0x%x-0x%x\n",
              user_stack, user_stack + 4*1024*1024);
    debug_debug("    Kernel Stack: 0x%x-0x%x\n",
              kernel_stack, kernel_stack + 1024*1024);

}
// fork系统调用实现
int ProcessManager::fork() {
    debug_debug("fork enter!\n");
    ProcessControlBlock* parent = &processes[current_pid];
    uint32_t child_pid = create_process(parent->name);
    if (child_pid == 0 && init_process(child_pid, parent->name)) {
        debug_err("Create process failed\n");
        return -1;
    }

    debug_debug("before fork parent pcb:");
    parent->print();

    ProcessControlBlock& child = processes[child_pid];

    // 复制父进程的寄存器状态
    // 复制寄存器状态
    child.eax = 0;  // 子进程返回0
    child.ebx = parent->ebx;
    child.ecx = parent->ecx;
    child.edx = parent->edx;
    child.esi = parent->esi;
    child.edi = parent->edi;
    child.esp = parent->esp;
    child.ebp = parent->ebp;

    // 复制段寄存器
    child.cs = parent->cs;
    child.ds = parent->ds;
    child.ss = parent->ss;
    child.es = parent->es;
    child.fs = parent->fs;
    child.gs = parent->gs;

    // 复制指令指针和标志寄存器
    child.eip = parent->eip;
    child.eflags = parent->eflags;


    // 复制内核态信息
    // child.esp0 = parent->esp0;
    // child.ebp0 = parent->ebp0;
    // child.ss0 = parent->ss0;
    

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

    // 复制文件描述符表
    for (int i = 0; i < 256; i++) {
        child.fd_table[i] = parent->fd_table[i];
    }

    // 复制进程状态和属性
    child.state = PROCESS_READY;
    child.priority = parent->priority;
    child.time_slice = parent->time_slice;
    child.total_time = 0;  // 新进程从0开始计时
    child.exit_status = 0;

    // 设置子进程状态为就绪
    child.state = PROCESS_READY;
    debug_debug("fork child pcb:");
    child.print();

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
        debug_debug("will schedule to new process: pcb:");
        processes[current_pid].print();
    }
    return true;
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
    current->cs = esp[13];
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
    esp[13] = next->cs;
    esp[12] = next->eip;

    // 恢复段寄存器
    esp[8] = next->ds;
    esp[9] = next->es;
    esp[10] = next->fs;
    esp[11] = next->gs;
    GDT::updateTSS(processes[current_pid].esp0, KERNEL_DS);
    GDT::updateTSSCR3(processes[current_pid].cr3);
}
void ProcessManager::switch_to_user_mode(uint32_t entry_point) {
    auto pcb = get_current_process();
    auto user_stack = pcb->user_stack+ pcb->user_stack_size;
    debug_debug("switch_to_user_mode called, user stack: %x, entry point %x\n", user_stack, entry_point);

    debug_debug("will switch to user: kernel pcb:");
    get_current_process()->print();

    pcb->eip = entry_point;
    pcb->esp = user_stack;
    pcb->ss = pcb->ds = USER_DS;
    pcb->eflags == 0x200;



    // asm volatile(
    //     "mov %%eax, %%esp\n\t"         // 设置用户栈指针
    //     "pushl $0x23\n\t"             // 用户数据段选择子
    //     "pushl %%eax\n\t"             // 用户栈指针
    //     "pushfl\n\t"                  // 原始EFLAGS
    //     "orl $0x200, (%%esp)\n\t"     // 开启中断标志
    //     "pushl $0x1B\n\t"             // 用户代码段选择子
    //     "pushl %%ebx\n\t"             // 入口地址
    //     "iret\n\t"                    // 切换特权级
    //     :
    //     : "a" (user_stack), "b" (entry_point)
    //     : "memory", "cc"
    // );
    // __builtin_unreachable();  // 避免编译器警告
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