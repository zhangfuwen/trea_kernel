#include "kernel/process.h"
#include "kernel/buddy_allocator.h"
#include <kernel/console_device.h>
#include <kernel/vfs.h>
#include <lib/console.h>

#include "arch/x86/gdt.h"
#include "kernel/process.h"
#include <cstdint>
#include <kernel/kernel.h>
#include <kernel/scheduler.h>
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
                    return i * 32 + j;
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
    PidManager::initialize();
}

void ProcessManager::initIdle()
{
    kernel_context = CURRENT();
    auto& pcb = kernel_context->context;
    strcpy(pcb.def_task.name, "idle");
    pcb.def_task.task_id = 0;
    pcb.def_task.state = PROCESS_RUNNING;
    pcb.def_task.priority = 0;
    pcb.def_task.time_slice = 100;
    pcb.def_task.total_time = 0;

    pcb.def_task.stacks.esp0 = (uint32_t)CURRENT() + KERNEL_STACK_SIZE;
    pcb.def_task.stacks.ss0 = KERNEL_DS;
    pcb.def_task.stacks.ebp0 = pcb.def_task.stacks.esp0;

    pcb.def_task.regs.cr3 = 0x400000;
    pcb.def_task.regs.ss = 0x10;

    pcb.def_task.next = &pcb.def_task;
    pcb.def_task.prev = &pcb.def_task;

    current_task = &pcb.def_task;

    debug_debug("idle pcb: %x\n", kernel_context);
    kernel_context->context.print();
}

kernel::ConsoleFS console_fs;

void* operator new(size_t size, void* ptr) noexcept;
void* operator new[](size_t size, void* ptr) noexcept;
// 创建新进程框架
Context* ProcessManager::create_context(const char* name)
{
    uint32_t pid = pid_manager.alloc();
    debug_info("creating process with pid: %d\n", pid);

    auto& kernel_mm = Kernel::instance().kernel_mm();
    auto addr = kernel_mm.alloc_pages(0, 2); // gfp_mask = 0, order = 2 (4 pages)
    auto context = new((void*)kernel_mm.phys2Virt(addr)) Context();
    auto task = context->def_task;
    task.context = context;

    // auto pcbAddr = &((PCB*)kernel_mm.alloc_pages(4))->pcb;
    // auto& pcb = *pcbAddr;
    strcpy(context->def_task.name, name);
    context->def_task.task_id = pid;
    context->def_task.state = PROCESS_NEW;
    context->def_task.priority = 1;
    context->def_task.time_slice = 100;
    context->def_task.total_time = 0;
    // 分配内核态栈
    context->def_task.stacks.ss0 = KERNEL_DS;
    auto kernel_stack = (uint8_t*)(context);
    context->def_task.stacks.esp0 = (uint32_t)kernel_stack + KERNEL_STACK_SIZE - 16;
    context->def_task.stacks.ebp0 = context->def_task.stacks.esp0;

    return context;
}

// 初始化进程详细信息
Task* ProcessManager::kernel_task(Context *context,
    const char* name, uint32_t entry, uint32_t argc, char* argv[])
{
    auto& kernel_mm = Kernel::instance().kernel_mm();

    auto pgd = Kernel::instance().kernel_mm().paging().getCurrentPageDirectory();
    debug_debug("ProcessManager: Current Page Directory: %x\n", pgd);
    auto &task = *new Task();

    // 清理用户栈
    task.stacks.user_stack = 0;
    task.stacks.user_stack_size = 0;

    // 初始化寄存器
    task.regs.eip = entry;
    task.regs.eflags = 0x200;

    task.regs.cs = KERNEL_CS;
    task.regs.ds = KERNEL_DS;
    task.regs.es = KERNEL_DS;
    task.regs.gs = KERNEL_DS;
    task.regs.fs = KERNEL_DS;
    task.regs.ss = KERNEL_DS;

    task.regs.esp = task.stacks.esp0;
    task.regs.ebp = task.stacks.ebp0;

    task.regs.cr3 = 0x400000;

    // 复制进程名称
    for(int i = 0; i < PROCNAME_LEN && name[i]; i++) {
        task.name[i] = name[i];
    }
    task.name[PROCNAME_LEN] = '\0';

    debug_debug("initialized task 0x%x, tid: %d\n", task, task.task_id);
    task.print();
    // appendPCB((PCB*)pPcb);
    return &task;
}
int Context::allocate_fd()
{
    auto ret = next_fd;
    debug_debug("allocate_fd: %x\n", ret);
    next_fd++;
    return ret;
}

void ProcessManager::cloneMemorySpace(Context* child, Context* parent)
{
    if(!child) {
        debug_err("ProcessManager: Invalid PCB pointer\n");
        return;
    }
    Kernel& kernel = Kernel::instance();
    auto& kernel_mm = kernel.kernel_mm();
    debug_debug("Copying memory space\n");
    // 使用COW方式复制内存空间
    PageDirectory* parent_pgd = (PageDirectory*)kernel_mm.phys2Virt(parent->def_task.regs.cr3);
    auto paddr = kernel_mm.alloc_pages(0, 0); // gfp_mask = 0, order = 0 (1 page)
    debug_debug("alloc page at 0x%x\n", paddr);
    auto child_pgd = kernel_mm.phys2Virt(paddr);
    debug_debug("child_pgd: 0x%x\n", child_pgd);
    PagingValidate(parent_pgd);
    debug_info("Copying memory space\n");
    kernel_mm.paging().copyMemorySpaceCOW(parent_pgd, (PageDirectory*)child_pgd);
    debug_debug("Copying page at 0x%x\n", paddr);
    child->def_task.regs.cr3 = paddr;
    child->user_mm.init((uint32_t)child_pgd,
        []() {
            auto page = Kernel::instance().kernel_mm().alloc_pages(
                0, 0); // gfp_mask = 0, order = 0 (1 page)
            debug_debug("ProcessManager: Allocated Page at %x\n", page);
            return page;
        },
        [](uint32_t physAddr) {
            Kernel::instance().kernel_mm().free_pages(physAddr, 0);
        }, // order=0表示释放单个页面
        [](uint32_t physAddr) {
            return (void*)Kernel::instance().kernel_mm().phys2Virt(physAddr);
        });
}

int ProcessManager::allocUserStack(Context* pcb)
{
    auto& mm = pcb->user_mm;
    pcb->def_task.stacks.user_stack_size = USER_STACK_SIZE;
    pcb->def_task.stacks.user_stack = (uint32_t)mm.allocate_area(
        pcb->def_task.stacks.user_stack_size, PAGE_WRITE, MEM_TYPE_STACK);
    debug_debug("user stack:0x%x\n", pcb->def_task.stacks.user_stack);
    printPDPTE((void*)pcb->def_task.stacks.user_stack);
    if(!pcb->def_task.stacks.user_stack) {
        debug_debug("ProcessManager: Failed to allocate user stack\n");
        return -1;
    }
    return 0;
}

void ProcessManager::appendPCB(Kontext* kontext)
{
    if(!kontext) {
        debug_err("ProcessManager: Invalid PCB pointer\n");
        return;
    }
    auto root = &kernel_context->context;
    auto tail = root->def_task.prev;
    tail->next = &kontext->context.def_task;
    kontext->context.def_task.prev = tail;
    kontext->context.def_task.next = &root->def_task;
    root->def_task.prev = &kontext->context.def_task;
}

kernel::ConsoleFS ProcessManager::console_fs;

void Registers::print()
{
    debug_info("  Registers:\n");
    debug_info("    EAX: 0x%x  EBX: 0x%x  ECX: 0x%x  EDX: 0x%x\n", eax, ebx, ecx, edx);
    debug_info("    ESI: 0x%x  EDI: 0x%x  EBP: 0x%x  ESP: 0x%x\n", esi, edi, ebp, esp);
    debug_info("    EIP: 0x%x  EFLAGS: 0x%x\n", eip, eflags);

    // 内核态信息
    debug_info("  Kernel State:\n");
    debug_info("    CR3: 0x%x\n", cr3);

    // 段寄存器
    debug_info("  Segment Selectors:\n");
    debug_info("    CS: 0x%x  DS: 0x%x  SS: 0x%x\n", cs, ds, ss);
    debug_info("    ES: 0x%x  FS: 0x%x  GS: 0x%x\n", es, fs, gs);
}

void Stacks::print()
{
    debug_info("  Stacks:\n");
    debug_info(
        "    User Stack: 0x%x, size:%d(0x%x)\n", user_stack, user_stack_size, user_stack_size);
    uint32_t kernel_stack = (uint32_t)(CURRENT()->stack);
    // debug_debug("    Kernel Stack: 0x%x, size:%d(0x%x)\n", kernel_stack, KERNEL_STACK_SIZE,
    //     KERNEL_STACK_SIZE);
    debug_info("    Esp0:0x%x, Ebp0:0x%x, Ss0:0x%x\n", esp0, ebp0, ss0);
}

void Context::print()
{
    def_task.print();
    user_mm.print();
}

void Task::print()
{
    debug_info("Process: %s (PID: %d), pcb_addr:0x%x\n", name, task_id, this);
    debug_info("  State: %d, Priority: %d, Time: %d/%d\n", state, priority, total_time, time_slice);

    regs.print();
    stacks.print();
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
    debug_info("fork enter!\n");
    Kernel& kernel = Kernel::instance();
    auto& kernel_mm = kernel.kernel_mm();

    Task* parent_task = ProcessManager::get_current_task();
    debug_info("before fork parent pcb:\n");
    parent_task->print();

    // 创建子进程
    debug_info("Creating child process\n");
    auto child = create_context(parent_task->name);
    if(child->def_task.task_id == 0) {
        debug_err("Create process failed\n");
        return -1;
    }

    debug_info("Copying parent process memory space\n");
    cloneMemorySpace(child, parent_task->context);
    child->user_mm.clone(parent_task->context->user_mm);

    debug_info("Copying parent process registers\n");
    auto& child_task = child->def_task;
    memcpy(&child_task.regs, &parent_task->regs, sizeof(Registers));
    child_task.regs.eax = 0; // 子进程返回0

    // 拷贝堆栈数据
    child_task.stacks.user_stack = parent_task->stacks.user_stack;
    child_task.stacks.user_stack_size = parent_task->stacks.user_stack_size;
    // 为栈分配新的物理页面
    uint8_t* start = (uint8_t*)child_task.stacks.user_stack;
    uint8_t* end = (uint8_t*)child_task.stacks.user_stack + child_task.stacks.user_stack_size;
    for(uint8_t* p = start; p < end; p += PAGE_SIZE) {
        copyCOWPage((uint32_t)p, parent_task->context->user_mm.getPageDirectory(),
            child_task.context->user_mm);
    }

    // 复制文件描述符表
    memcpy(&child_task.context->fd_table, &parent_task->context->fd_table,
        sizeof(child_task.context->fd_table));

    // 复制进程状态和属性
    child_task.state = PROCESS_READY;
    child_task.priority = parent_task->priority;
    child_task.time_slice = parent_task->time_slice;
    child_task.total_time = 0; // 新进程从0开始计时
    child_task.exit_status = 0;

    // 设置子进程状态为就绪
    child_task.state = PROCESS_READY;
    debug_info("fork child_task.pcb:\n");
    child_task.print();

    appendPCB((Kontext*)child);

    debug_info("fork return, parent: %d, child:%d\n", parent_task->task_id, child_task.task_id);
    return child_task.task_id;
}

// 切换到下一个进程
bool ProcessManager::schedule()
{
    auto current = (Kontext*)current_task;
    // debug_debug("schedule enter, current pcb 0x%x!\n", current);
    auto next = current->context.def_task.next;
    while(next != &current->context.def_task) {
        // debug_debug("schedule called, current: %d, next:%d, nextstate:%d\n", current->pcb.pid,
        // next->pid, next->state);
        if(next->state == PROCESS_READY || next->state == PROCESS_RUNNING) {
            break;
        }
        if(next->state == PROCESS_SLEEPING || next->sleep_ticks <= 0) {
            next->state = PROCESS_RUNNING;
            next->sleep_ticks = 0;
            break;
        } else {
            next->sleep_ticks--;
        }
        next = next->next;
    }
    if(next == &current->context.def_task) {
        debug_debug("equal process: 0x%x, pid:%d\n", next, next->task_id);
        current_task = next;
        return false; // 所有进程都处于就绪状态，无需切换
    } else {
        next->state = PROCESS_RUNNING;
        // debug_debug("switch to next process: %d\n", next->pid);
        current_task = next;
        // debug_debug("will schedule to new process: pcb:\n");
        // next->print();
    }
    return true;
}

// 保存当前进程的寄存器状态（由中断处理程序调用）
void ProcessManager::save_context(uint32_t* esp)
{
    // if (current_pid == 0) return;

    Task* current = current_task;
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
    if(regs.cs != 0x08 && regs.cs != 0x1b) {
        debug_debug("cs error 0x%x\n", regs.cs);
        current->print();
        current->debug_status |= DEBUG_STATUS_HALT;
    }
    // debug_debug("save context\n");
    // current->print();

    // debug_debug("saved context\n");
    // current->print();
}

// 恢复新进程的寄存器状态（由中断处理程序调用）
void ProcessManager::restore_context(uint32_t* esp)
{
    Task* next = current_task;
    auto& regs = next->regs;
    // if(next != &CURRENT()->pcb) {
    //     debug_debug("restore context called, current:0x%x, next_pcb: 0x%x, pid: %d\n", CURRENT(),
    //     next, next->pid); next->print();
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

    if(regs.cs != 0x08 && regs.cs != 0x1b) {
        debug_debug("cs error 0x%x\n", regs.cs);
        next->print();
        if(next->debug_status & DEBUG_STATUS_HALT) {
            while(true) {
                asm volatile("hlt");
            }
        }
    }
    next->debug_status = 0;

    // 恢复段寄存器
    esp[8] = regs.ds;
    esp[9] = regs.es;
    esp[10] = regs.fs;
    esp[11] = regs.gs;
    GDT::updateTSS(next->stacks.esp0, KERNEL_DS);
    GDT::updateTSSCR3(next->regs.cr3);
}
Task* ProcessManager::get_current_task() { return current_task; }

void ProcessManager::switch_to_user_mode(uint32_t entry_point, Context* context)
{
    // auto pcb = get_current_process();
    auto task = &context->def_task;
    auto user_stack = task->stacks.user_stack + task->stacks.user_stack_size - 16;
    debug_debug(
        "switch_to_user_mode called, user stack: %x, entry point %x\n", user_stack, entry_point);

    debug_debug("will switch to user: kernel pcb:");
    get_current_task()->print();

    task->regs.eip = entry_point;
    task->regs.esp = user_stack;
    task->regs.ss = task->regs.ds = USER_DS;
    task->regs.eflags == 0x200;

    __printPDPTE((void*)entry_point, (PageDirectory*)context->user_mm.getPageDirectory());
    __printPDPTE((void*)user_stack, (PageDirectory*)context->user_mm.getPageDirectory());
    GDT::updateTSS(task->stacks.esp0, KERNEL_DS);
    GDT::updateTSSCR3(task->regs.cr3);

    asm volatile("mov %%eax, %%esp\n\t"    // 设置用户栈指针
                 "pushl $0x23\n\t"         // 用户数据段选择子
                 "pushl %%eax\n\t"         // 用户栈指针
                 "pushfl\n\t"              // 原始EFLAGS
                 "orl $0x200, (%%esp)\n\t" // 开启中断标志
                 "pushl $0x1B\n\t"         // 用户代码段选择子
                 "pushl %%ebx\n\t"         // 入口地址
                 "iret\n\t"                // 切换特权级
        :
        : "a"(user_stack), "b"(entry_point)
        : "memory", "cc");
    // __builtin_unreachable();  // 避免编译器警告
}

// 静态成员初始化
Kontext* ProcessManager::kernel_context;
Task* ProcessManager::current_task = nullptr;

void ProcessManager::sleep_current_process(uint32_t ticks)
{
    Task* task = get_current_task();
    if(!task)
        return;

    task->state = PROCESS_SLEEPING; // 需要确保枚举值已定义
    task->sleep_ticks = ticks;      // 需要在PCB结构中添加该字段

    // 触发调度让出CPU
    schedule();
}