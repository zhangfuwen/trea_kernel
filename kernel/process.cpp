#include "kernel/process.h"
#include "kernel/buddy_allocator.h"
#include <kernel/console_device.h>
#include <kernel/vfs.h>
#include <lib/console.h>

#include "arch/x86/gdt.h"
#include "kernel/process.h"
#include <cstdint>
#include <kernel/kernel.h>
#include <lib/debug.h>
#include <lib/string.h>

uint32_t PidManager::pid_bitmap[(PidManager::MAX_PID + 31) / 32];

int32_t PidManager::alloc()
{
    for(uint32_t i = 0; i < (MAX_PID + 31) / 32; i++) {
        if(pid_bitmap[i] != 0xFFFFFFFF) {
            for(int j = 0; j < 32; j++) {
                uint32_t mask = 1 << j;
                if(!(pid_bitmap[i] & mask)) {
                    pid_bitmap[i] |= mask;
                    return i * 32 + j + 1;
                }
            }
        }
    }
    return -1;
}

void PidManager::free(uint32_t pid)
{
    if(pid == 0 || pid > MAX_PID)
        return;
    pid--;
    uint32_t i = pid / 32;
    uint32_t j = pid % 32;
    pid_bitmap[i] &= ~(1 << j);
}

void PidManager::initialize()
{
    memset(pid_bitmap, 0, sizeof(pid_bitmap));
    pid_bitmap[0] |= 1; // 保留PID 0
}

void ProcessManager::init()
{
    Console::print("ProcessManager initialized\n");
}

void ProcessManager::initIdle()
{
    idle_pcb = CURRENT();
    auto& pcb = idle_pcb->pcb;
    pcb.pid = 0;
    pcb.state = PROCESS_RUNNING;
    pcb.priority = 0;
    pcb.time_slice = 100;
    pcb.total_time = 0;

    pcb.stacks.esp0 = (uint32_t)CURRENT() + KERNEL_STACK_SIZE;
    pcb.stacks.ss0 = KERNEL_DS;
    pcb.stacks.ebp0 = pcb.stacks.esp0;

    pcb.regs.cr3 = 0x400000;
    pcb.regs.ss = 0x10;

    pcb.next = &pcb;
    pcb.prev = &pcb;

    current_pcb = &pcb;

    debug_debug("idle pcb: %x\n", idle_pcb);
    idle_pcb->pcb.print();
}

kernel::ConsoleFS console_fs;

// 创建新进程框架
ProcessControlBlock* ProcessManager::create_process(const char* name)
{
    uint32_t pid = pid_manager.alloc();

    auto kernel_mm = Kernel::instance().kernel_mm();

    auto pcbAddr = &((PCB*)kernel_mm.alloc_pages(4))->pcb;
    auto& pcb = *pcbAddr;
    pcb.pid = pid;
    pcb.state = PROCESS_NEW;
    pcb.priority = 1;
    pcb.time_slice = 100;
    pcb.total_time = 0;

    pcb.user_mm.init((uint32_t)Kernel::instance().kernel_mm().paging().getCurrentPageDirectory(),
        []() {
            auto page = Kernel::instance().kernel_mm().allocPage();
            debug_debug("ProcessManager: Allocated Page at %x\n", page);
            return page;
        },
        [](uint32_t physAddr) { Kernel::instance().kernel_mm().freePage(physAddr); },
        [](uint32_t physAddr) {
            return (void*)Kernel::instance().kernel_mm().phys2Virt(physAddr);
        });

    return pcbAddr;
}

// 初始化进程详细信息
int ProcessManager::kernel_process(const char* name, uint32_t entry, uint32_t argc, char* argv[])
{
    auto pPcb = create_process(name);
    auto& pcb = *pPcb;
    auto kernel_mm = Kernel::instance().kernel_mm();

    auto pgd = Kernel::instance().kernel_mm().paging().getCurrentPageDirectory();
    debug_debug("ProcessManager: Current Page Directory: %x\n", pgd);

    // 初始化内存管理
    pcb.state = PROCESS_NEW;
    pcb.priority = 1;
    pcb.time_slice = 100;
    pcb.total_time = 0;

    // 分配内核态栈
    pcb.stacks.ss0 = KERNEL_DS;
    auto kernel_stack = (uint8_t*)(pPcb);
    pcb.stacks.esp0 = (uint32_t)kernel_stack + KERNEL_STACK_SIZE - 16;
    pcb.stacks.ebp0 = pcb.stacks.esp0;
    pcb.stacks.user_stack = 0;
    pcb.stacks.user_stack_size = 0;

    // 初始化寄存器
    pcb.regs.eip = entry;
    pcb.regs.eflags = 0x200;

    pcb.regs.cs = KERNEL_CS;
    pcb.regs.ds = KERNEL_DS;
    pcb.regs.es = KERNEL_DS;
    pcb.regs.gs = KERNEL_DS;
    pcb.regs.fs = KERNEL_DS;
    pcb.regs.ss = KERNEL_DS;

    pcb.regs.esp = pcb.stacks.esp0;
    pcb.regs.ebp = pcb.stacks.ebp0;

    pcb.regs.cr3 = 0x400000;

    // 复制进程名称
    for(int i = 0; i < PROCNAME_LEN && name[i]; i++) {
        pcb.name[i] = name[i];
    }
    pcb.name[PROCNAME_LEN] = '\0';

    // 初始化标准输入输出流
    pcb.fd_table[0] = console_fs.open("/dev/console");
    pcb.fd_table[1] = console_fs.open("/dev/console");
    pcb.fd_table[2] = console_fs.open("/dev/console");

    debug_debug("initialized process 0x%x, pid: %d\n", pPcb, pcb.pid);
    pcb.print();
    appendPCB((PCB*)pPcb);
    pcb.state = PROCESS_READY;
    return pcb.pid;
}

void ProcessManager::appendPCB(PCB* pcb)
{
    if(!pcb) {
        debug_err("ProcessManager: Invalid PCB pointer\n");
        return;
    }
    auto root = &idle_pcb->pcb;
    auto tail = root->prev;
    tail->next = &pcb->pcb;
    pcb->pcb.prev = tail;
    pcb->pcb.next = root;
    root->prev = &pcb->pcb;
}

kernel::ConsoleFS ProcessManager::console_fs;

void Registers::print()
{
    debug_debug("  Registers:\n");
    debug_debug("    EAX: 0x%x  EBX: 0x%x  ECX: 0x%x  EDX: 0x%x\n", eax, ebx, ecx, edx);
    debug_debug("    ESI: 0x%x  EDI: 0x%x  EBP: 0x%x  ESP: 0x%x\n", esi, edi, ebp, esp);
    debug_debug("    EIP: 0x%x  EFLAGS: 0x%x\n", eip, eflags);

    // 内核态信息
    debug_debug("  Kernel State:\n");
    debug_debug("    CR3: 0x%x\n", cr3);

    // 段寄存器
    debug_debug("  Segment Selectors:\n");
    debug_debug("    CS: 0x%x  DS: 0x%x  SS: 0x%x\n", cs, ds, ss);
    debug_debug("    ES: 0x%x  FS: 0x%x  GS: 0x%x\n", es, fs, gs);
}

void Stacks::print()
{
    debug_debug("  Stacks:\n");
    debug_debug(
        "    User Stack: 0x%x, size:%d(0x%x)\n", user_stack, user_stack_size, user_stack_size);
    uint32_t kernel_stack = (uint32_t)(CURRENT()->stack);
    // debug_debug("    Kernel Stack: 0x%x, size:%d(0x%x)\n", kernel_stack, KERNEL_STACK_SIZE,
    //     KERNEL_STACK_SIZE);
    debug_debug("    Esp0:0x%x, Ebp0:0x%x, Ss0:0x%x\n", esp0, ebp0, ss0);
}

void ProcessControlBlock::print()
{
    debug_debug("Process: %s (PID: %d)\n", name, pid);
    debug_debug(
        "  State: %d, Priority: %d, Time: %d/%d\n", state, priority, total_time, time_slice);

    regs.print();
    stacks.print();
    user_mm.print();
}

/**
 * @brief 分配栈空间
 *
 * @return int
 */
int Stacks::allocSpace(UserMemory& mm)
{
    user_stack_size = USER_STACK_SIZE;
    user_stack = (uint32_t)mm.allocate_area(user_stack_size, PAGE_WRITE, MEM_TYPE_STACK);
    if(!user_stack) {
        debug_debug("ProcessManager: Failed to allocate user stack\n");
        return -1;
    }
    debug_debug("Child user stack allocated at 0x%x, size:%d(0x%x)\n", user_stack, user_stack_size,
        user_stack_size);

    return 0;
}

// fork系统调用实现
// 修改fork中的内存复制逻辑
int ProcessManager::fork()
{
    debug_debug("fork enter!\n");
    Kernel& kernel = Kernel::instance();
    auto& kernel_mm = kernel.kernel_mm();

    ProcessControlBlock* parent = &CURRENT()->pcb;
    debug_debug("before fork parent pcb:\n");
    parent->print();

    // 创建子进程
    debug_debug("Creating child process\n");
    auto child = create_process(parent->name);
    if(child->pid == 0) {
        debug_err("Create process failed\n");
        return -1;
    }

    debug_debug("Copying parent process registers\n");
    memcpy(&child->regs, &parent->regs, sizeof(Registers));
    child->regs.eax = 0; // 子进程返回0

    debug_debug("Copying parent process stacks\n");
    if(auto ret = child->stacks.allocSpace(child->user_mm); ret != 0) {
        debug_err("alloc space failed, ret:%d\n", ret);
        return -1;
    }
    // 拷贝堆栈数据
    memcpy((void*)child->stacks.user_stack, (void*)parent->stacks.user_stack,
        parent->stacks.user_stack_size);
    child->stacks.esp0 = (uint32_t)((uint8_t*)&child + KERNEL_STACK_SIZE - 16);
    child->stacks.ebp0 = child->stacks.esp0;
    child->stacks.ss0 = KERNEL_DS;

    debug_debug("Copying memory space\n");
    // 使用COW方式复制内存空间
    PageDirectory* parent_pgd = (PageDirectory*)kernel_mm.phys2Virt(parent->regs.cr3);
    auto paddr = kernel_mm.allocPage();
    debug_debug("alloc page at 0x%x\n", paddr);
    auto child_pgd = kernel_mm.phys2Virt(paddr);
    debug_debug("child_pgd: 0x%x\n", child_pgd);
    kernel_mm.paging().copyMemorySpaceCOW(parent_pgd, (PageDirectory*)child_pgd);
    debug_debug("Copying page at 0x%x\n", paddr);
    child->regs.cr3 = paddr;

    // 复制文件描述符表
    for(int i = 0; i < 256; i++) {
        child->fd_table[i] = parent->fd_table[i];
    }

    // 复制进程状态和属性
    child->state = PROCESS_READY;
    child->priority = parent->priority;
    child->time_slice = parent->time_slice;
    child->total_time = 0; // 新进程从0开始计时
    child->exit_status = 0;

    // 设置子进程状态为就绪
    child->state = PROCESS_READY;
    debug_debug("fork child->pcb:\n");
    child->print();

    debug_debug("fork return, parent: %d, child:%d\n", parent->pid, child->pid);
    return child->pid;
}

// 切换到下一个进程
bool ProcessManager::schedule()
{
    auto current = (PCB*)current_pcb;
    debug_debug("schedule enter, current pcb 0x%x!\n", current);
    auto next = current->pcb.next;
    while(next != &current->pcb) {
        debug_debug("schedule called, current: %d, next:%d, nextstate:%d\n", current->pcb.pid,
            next->pid, next->state);
        if(next->state == PROCESS_READY || next->state == PROCESS_RUNNING) {
            break;
        }
        next = next->next;
    }
    if(next == &current->pcb) {
        debug_debug("equal process: 0x%x, pid:%d\n", next, next->pid);
        current_pcb = next;
        return false; // 所有进程都处于就绪状态，无需切换
    } else {
        next->state = PROCESS_RUNNING;
        debug_debug("switch to next process: %d\n", next->pid);
        current_pcb = next;
        debug_debug("will schedule to new process: pcb:\n");
        next->print();
    }
    return true;
}

// 保存当前进程的寄存器状态（由中断处理程序调用）
void ProcessManager::save_context(uint32_t* esp)
{
    // if (current_pid == 0) return;

    ProcessControlBlock* current = current_pcb;
    auto& regs = current->regs;
    // 从中断栈中获取寄存器状态
    regs.eax = esp[7]; // pushad的顺序：eax,ecx,edx,ebx,esp,ebp,esi,edi
    regs.ecx = esp[6];
    regs.edx = esp[5];
    regs.ebx = esp[4];
    regs.esp = esp[3];
    regs.ebp = esp[2];
    regs.esi = esp[1];
    regs.edi = esp[0];

    // 获取eflags和eip（从中断栈中）
    regs.eflags = esp[14]; // 中断栈：gs,fs,es,ds,pushad,vector,error,eip,cs,eflags
    regs.cs = esp[13];
    regs.eip = esp[12];

    // 保存段寄存器
    regs.ds = esp[8];
    regs.es = esp[9];
    regs.fs = esp[10];
    regs.gs = esp[11];

    // debug_debug("saved context\n");
    // current->print();
}

// 恢复新进程的寄存器状态（由中断处理程序调用）
void ProcessManager::restore_context(uint32_t* esp)
{
    ProcessControlBlock* next = current_pcb;
    auto& regs = next->regs;
    // if(next != &CURRENT()->pcb) {
    //     debug_debug("restore context called, current:0x%x, next_pcb: 0x%x, pid: %d\n", CURRENT(), next, next->pid);
    //     next->print();
    // }

    // 恢复通用寄存器
    esp[7] = regs.eax;
    esp[6] = regs.ecx;
    esp[5] = regs.edx;
    esp[4] = regs.ebx;
    esp[3] = regs.esp;
    esp[2] = regs.ebp;
    esp[1] = regs.esi;
    esp[0] = regs.edi;

    // 恢复eflags和eip
    esp[14] = regs.eflags;
    esp[13] = regs.cs;
    esp[12] = regs.eip;

    // 恢复段寄存器
    esp[8] = regs.ds;
    esp[9] = regs.es;
    esp[10] = regs.fs;
    esp[11] = regs.gs;
    GDT::updateTSS(next->stacks.esp0, KERNEL_DS);
    GDT::updateTSSCR3(next->regs.cr3);
}
ProcessControlBlock* ProcessManager::get_current_process()
{
    return current_pcb;
}

void ProcessManager::switch_to_user_mode(uint32_t entry_point)
{
    auto pcb = get_current_process();
    auto user_stack = pcb->stacks.user_stack + pcb->stacks.user_stack_size;
    debug_debug(
        "switch_to_user_mode called, user stack: %x, entry point %x\n", user_stack, entry_point);

    debug_debug("will switch to user: kernel pcb:");
    get_current_process()->print();

    pcb->regs.eip = entry_point;
    pcb->regs.esp = user_stack;
    pcb->regs.ss = pcb->regs.ds = USER_DS;
    pcb->regs.eflags == 0x200;

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

// 静态成员初始化
PCB* ProcessManager::idle_pcb;
ProcessControlBlock* ProcessManager::current_pcb = nullptr;
