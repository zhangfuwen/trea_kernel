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
}

void ProcessManager::init()
{
    Console::print("ProcessManager initialized\n");
    PidManager::initialize();
}

kernel::ConsoleFS console_fs;

void* operator new(size_t size, void* ptr) noexcept;
void* operator new[](size_t size, void* ptr) noexcept;
// 创建新进程框架
Context* ProcessManager::create_context(const char* name)
{
    auto& kernel_mm = Kernel::instance().kernel_mm();

    uint32_t pid = pid_manager.alloc();
    log_info("creating process with pid: %d\n", pid);

    auto context = new Context();
    context->context_id = pid;


    return context;
}

// 初始化进程详细信息
Task* ProcessManager::kernel_task(
    Context* context, const char* name, uint32_t entry, uint32_t argc, char* argv[])
{
    auto& kernel_mm = Kernel::instance().kernel_mm();

    auto pgd = Kernel::instance().kernel_mm().paging().getCurrentPageDirectory();
    log_debug("ProcessManager: Current Page Directory: %x\n", pgd);
    auto task = new Task();
    task->task_id = tid_manager.alloc();
    task->context = context;

    task->state = PROCESS_READY;
    task->priority = 0;
    task->time_slice = DEFAULT_TIME_SLICE;
    task->total_time = 0; // 新进程从0开始计时
    task->exit_status = 0;

    // 清理用户栈
    task->stacks.user_stack = 0;
    task->stacks.user_stack_size = 0;

    // 初始化寄存器
    task->regs.eip = entry;
    task->regs.eflags = 0x200;

    task->regs.cs = KERNEL_CS;
    task->regs.ds = KERNEL_DS;
    task->regs.es = KERNEL_DS;
    task->regs.gs = KERNEL_DS;
    task->regs.fs = KERNEL_DS;
    task->regs.ss = KERNEL_DS;

    task->regs.esp = task->stacks.esp0;
    task->regs.ebp = task->stacks.ebp0;

    task->regs.cr3 = 0x400000;

    task->alloc_stack(kernel_mm);

    // 复制进程名称
    for(int i = 0; i < PROCNAME_LEN && name[i]; i++) {
        task->name[i] = name[i];
    }
    task->name[PROCNAME_LEN] = '\0';

    log_debug("initialized task:0x%x, tid: %d\n", task, task->task_id);
    task->print();
    // appendPCB((PCB*)pPcb);
    return task;
}
int Context::allocate_fd()
{
    auto ret = next_fd;
    log_debug("allocate_fd: %x\n", ret);
    next_fd++;
    return ret;
}

kernel::ConsoleFS ProcessManager::console_fs;

void Registers::print()
{
    log_info("  Registers:\n");
    log_info("    EAX: 0x%x  EBX: 0x%x  ECX: 0x%x  EDX: 0x%x\n", eax, ebx, ecx, edx);
    log_info("    ESI: 0x%x  EDI: 0x%x  EBP: 0x%x  ESP: 0x%x\n", esi, edi, ebp, esp);
    log_info("    EIP: 0x%x  EFLAGS: 0x%x\n", eip, eflags);

    // 内核态信息
    log_info("  Kernel State:\n");
    log_info("    CR3: 0x%x\n", cr3);

    // 段寄存器
    log_info("  Segment Selectors:\n");
    log_info("    CS: 0x%x  DS: 0x%x  SS: 0x%x\n", cs, ds, ss);
    log_info("    ES: 0x%x  FS: 0x%x  GS: 0x%x\n", es, fs, gs);
}

void Stacks::print()
{
    log_info("  Stacks:\n");
    log_info(
        "    User Stack: 0x%x, size:%d(0x%x)\n", user_stack, user_stack_size, user_stack_size);
    // debug_debug("    Kernel Stack: 0x%x, size:%d(0x%x)\n", kernel_stack, KERNEL_STACK_SIZE,
    //     KERNEL_STACK_SIZE);
    log_info("    Esp0:0x%x, Ebp0:0x%x, Ss0:0x%x\n", esp0, ebp0, ss0);
}

void Context::print()
{
    user_mm.print();
}
void Context::cloneFiles(Context* source)
{
    memcpy(fd_table, source->fd_table, sizeof(fd_table));
}


void Context::cloneMemorySpace(Context* source)
{
    if(!source) {
        log_err("ProcessManager: Invalid PCB pointer\n");
        return;
    }
    log_debug("Copying memory space\n");

    Kernel& kernel = Kernel::instance();
    auto& kernel_mm = kernel.kernel_mm();

    // 使用COW方式复制内存空间
    auto parent_pgd = source->user_mm.getPageDirectory();
    auto paddr = kernel_mm.alloc_pages(0, 0); // gfp_mask = 0, order = 0 (1 page)
    log_debug("alloc page at 0x%x\n", paddr);
    auto child_pgd = kernel_mm.phys2Virt(paddr);
    log_debug("child_pgd: 0x%x\n", child_pgd);
    PagingValidate((PageDirectory*)parent_pgd);
    log_info("Copying memory space\n");
    kernel_mm.paging().copyMemorySpaceCOW((PageDirectory*)parent_pgd, (PageDirectory*)child_pgd);
    log_debug("Copying page at 0x%x\n", paddr);
    user_mm.init(
        paddr, child_pgd,
        []() {
            auto page = Kernel::instance().kernel_mm().alloc_pages(
                0, 0); // gfp_mask = 0, order = 0 (1 page)
            log_debug("ProcessManager: Allocated Page at %x\n", page);
            return page;
        },
        [](uint32_t physAddr) {
            Kernel::instance().kernel_mm().free_pages(physAddr, 0);
        }, // order=0表示释放单个页面
        [](uint32_t physAddr) {
            return (void*)Kernel::instance().kernel_mm().phys2Virt(physAddr);
        });
}


void Task::print()
{
    log_info("Process: %s (PID: %d), pcb_addr:0x%x\n", name, task_id, this);
    log_info("  State: %d, Priority: %d, Time: %d/%d\n", state, priority, total_time, time_slice);

    regs.print();
    stacks.print();
}

void Task::alloc_stack(KernelMemory& kernel_mm)
{
    stacks.ss0 = KERNEL_DS;
    auto kernel_stack = kernel_mm.kmalloc(KERNEL_STACK_SIZE);
    stacks.kernel_stack = kernel_stack;
    stacks.esp0 = (uint32_t)kernel_stack + KERNEL_STACK_SIZE - 16;
    stacks.ebp0 = stacks.esp0;
}

int Task::allocUserStack()
{
    auto& mm = context->user_mm;
    stacks.user_stack_size = USER_STACK_SIZE;
    stacks.user_stack =
        (uint32_t)mm.allocate_area(stacks.user_stack_size, PAGE_WRITE, MEM_TYPE_STACK);
    log_debug("user stack:0x%x\n", stacks.user_stack);
    printPDPTE((void*)stacks.user_stack);
    if(!stacks.user_stack) {
        log_debug("ProcessManager: Failed to allocate user stack\n");
        return -1;
    }
    return 0;
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
        log_debug("ProcessManager: Failed to allocate user stack\n");
        return -1;
    }
    log_debug("Child user stack allocated at 0x%x, size:%d(0x%x)\n", user_stack, user_stack_size,
        user_stack_size);

    return 0;
}

// fork系统调用实现
// 修改fork中的内存复制逻辑
int ProcessManager::fork()
{
    log_info("fork enter!\n");
    // Kernel& kernel = Kernel::instance();
    // auto& kernel_mm = kernel.kernel_mm();
    //
    // Task* parent_task = ProcessManager::get_current_task();
    // debug_info("before fork parent pcb:\n");
    // parent_task->print();
    //
    // // 创建子进程
    // debug_info("Creating child process\n");
    // auto child = create_context(parent_task->name);
    // if(child->def_task.task_id == 0) {
    //     debug_err("Create process failed\n");
    //     return -1;
    // }
    //
    // debug_info("Copying parent process memory space\n");
    // cloneMemorySpace(child, parent_task->context);
    // child->user_mm.clone(parent_task->context->user_mm);
    //
    // debug_info("Copying parent process registers\n");
    // auto& child_task = child->def_task;
    // memcpy(&child_task.regs, &parent_task->regs, sizeof(Registers));
    // child_task.regs.eax = 0; // 子进程返回0
    //
    // // 拷贝堆栈数据
    // child_task.stacks.user_stack = parent_task->stacks.user_stack;
    // child_task.stacks.user_stack_size = parent_task->stacks.user_stack_size;
    // // 为栈分配新的物理页面
    // uint8_t* start = (uint8_t*)child_task.stacks.user_stack;
    // uint8_t* end = (uint8_t*)child_task.stacks.user_stack + child_task.stacks.user_stack_size;
    // for(uint8_t* p = start; p < end; p += PAGE_SIZE) {
    //     copyCOWPage((uint32_t)p, parent_task->context->user_mm.getPageDirectory(),
    //         child_task.context->user_mm);
    // }
    //
    // // 复制文件描述符表
    // memcpy(&child_task.context->fd_table, &parent_task->context->fd_table,
    //     sizeof(child_task.context->fd_table));
    //
    // // 复制进程状态和属性
    // child_task.state = PROCESS_READY;
    // child_task.priority = parent_task->priority;
    // child_task.time_slice = parent_task->time_slice;
    // child_task.total_time = 0; // 新进程从0开始计时
    // child_task.exit_status = 0;
    //
    // // 设置子进程状态为就绪
    // child_task.state = PROCESS_READY;
    // debug_info("fork child_task.pcb:\n");
    // child_task.print();
    //
    // appendPCB((Kontext*)child);
    //
    // debug_info("fork return, parent: %d, child:%d\n", parent_task->task_id, child_task.task_id);
    // return child_task.task_id;
    return 0;
}

// 切换到下一个进程
bool ProcessManager::schedule()
{
    auto current = get_current_task();
    if(current && --current->time_slice > 0) {
        //debug_debug("time_slice %d\n", current->time_slice);
        return false;
    }
    auto next = Kernel::instance().scheduler().pick_next_task();
    if(!next || next == current) {
        return false;
    }
    auto cpu = arch::apic_get_id();
    // log_debug("got next task: %d(0x%x, pre_cpu:%d), cpu: %d\n", next->task_id, next, next->cpu, cpu);
    // debug_debug("schedule: current: %d, next:%d(0x%x)\n", current->task_id, next->task_id, next);
    current->time_slice = DEFAULT_TIME_SLICE;
    Kernel::instance().scheduler().enqueue_task(current);
    Kernel::instance().scheduler().set_current_task(next);
    debug.is_task_switch = true;
    debug.cur_task = next;
    debug.prev_task = current;
    return true;
}

// 保存当前进程的寄存器状态（由中断处理程序调用）
void ProcessManager::save_context(uint32_t int_num, uint32_t* esp)
{
    // if (current_pid == 0) return;

    Task* current = Kernel::instance().scheduler().get_current_task();
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
    if((regs.cs != 0x08 && regs.cs != 0x1b) || regs.ds == 0) {
        log_debug("cs 0x%x, ds 0x%x, int 0x%x\n", regs.cs, regs.ds, int_num);
        current->print();
        current->debug_status |= DEBUG_STATUS_HALT;
    }
    // debug_debug("save context\n");
    // current->print();

    // debug_debug("saved context\n");
    // current->print();
}

// 恢复新进程的寄存器状态（由中断处理程序调用）
void ProcessManager::restore_context(uint32_t int_num, uint32_t* esp)
{
    //中断返回的可以是用户态，也可能是内核态
    // 内核态的典型场景是内核在执行idle任务。

    Task* next = get_current_task();
    auto& regs = next->regs;

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
        log_debug("cs error 0x%x, int_num 0x%x\n", regs.cs, int_num);
        next->print();
        if(next->debug_status & DEBUG_STATUS_HALT) {
            while(true) {
                asm volatile("hlt");
            }
        }
    }
    next->debug_status = 0;

    // 恢复段寄存器
    if(regs.ds ==0) {
        log_debug("ds error 0x%x, int_num 0x%x\n", regs.ds, int_num);
        next->print();
    }
    esp[8] = regs.ds;
    esp[9] = regs.es;
    esp[10] = regs.fs;
    esp[11] = regs.gs;
    auto cpu = arch::apic_get_id();
    GDT::updateTSS(cpu, next->stacks.esp0, KERNEL_DS);
    GDT::updateTSSCR3(cpu, next->regs.cr3);

    if(debug.is_task_switch && debug.cur_task->cpu != cpu) {
        log_trace("switching task: prev: %d, next:%d\n", debug.prev_task->task_id,
            next->task_id);
        log_trace("prev cpu:%d, cur cpu:%d\n", next->cpu, cpu);
        debug.is_task_switch = false;
        // next->print();
    }
    next->cpu = cpu;
    // update cr3
    asm volatile("mov %%eax, %%cr3\n\t" ::"a"(next->regs.cr3));
}
Task* ProcessManager::get_current_task()
{
    return Kernel::instance().scheduler().get_current_task();
}

void ProcessManager::switch_to_user_mode(uint32_t entry_point, Task* task)
{
    // auto pcb = get_current_process();
    auto user_stack = task->stacks.user_stack + task->stacks.user_stack_size - 16;
    auto context = task->context;
    log_debug(
        "switch_to_user_mode called, user stack: %x, entry point %x\n", user_stack, entry_point);

    log_debug("will switch to user: kernel pcb:\n");
    get_current_task()->print();

    task->regs.eip = entry_point;
    task->regs.esp = user_stack;
    task->regs.ss = task->regs.ds = USER_DS;
    task->regs.eflags == 0x200;

    __printPDPTE((void*)entry_point, (PageDirectory*)context->user_mm.getPageDirectory());
    log_debug("user stack: 0x%x\n", user_stack);
    __printPDPTE((void*)user_stack, (PageDirectory*)context->user_mm.getPageDirectory());
    auto cpu = arch::apic_get_id();
    GDT::updateTSS(cpu, task->stacks.esp0, KERNEL_DS);
    GDT::updateTSSCR3(cpu, task->regs.cr3);

    // // update ds and es
    // asm volatile("mov %0, %%ds\n\t"
    //              "mov %0, %%es\n\t"
    //              "mov %0, %%fs\n\t"
    //              "mov %0, %%gs\n\t"
    //     :
    //     : "r"(USER_DS), "r"(USER_DS), "r"(USER_DS), "r"(USER_DS));

    asm volatile("mov %%eax, %%esp\n\t"    // 设置用户栈指针
                 "pushl $0x23\n\t"         // 用户数据段选择子
                 "pushl %%eax\n\t"         // 用户栈指针
                 "pushfl\n\t"              // 原始EFLAGS
                 "orl $0x200, (%%esp)\n\t" // 开启中断标志
                 "pushl $0x1B\n\t"         // 用户代码段选择子
                 "pushl %%ebx\n\t"         // 入口地址
                 "mov %%cx, %%ds\n\t"         // 入口地址
                 "iret\n\t"                // 切换特权级
        :
        : "a"(user_stack), "b"(entry_point), "c"(0x23)
        : "memory", "cc");
    // __builtin_unreachable();  // 避免编译器警告
}

// 静态成员初始化
Context* ProcessManager::kernel_context = nullptr;

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

ProcessManager::Debug ProcessManager::debug;