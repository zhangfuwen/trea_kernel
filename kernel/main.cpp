#include <lib/ioport.h>

#include "arch/x86/gdt.h"
#include "arch/x86/idt.h"
#include "arch/x86/interrupt.h"
#include "arch/x86/paging.h"
#include "lib/console.h"
#include "lib/serial.h"
#include "kernel/process.h"
#include "kernel/scheduler.h"
#include "kernel/elf_loader.h"
#include "unistd.h"
#include "kernel/memfs.h"
#include "kernel/vfs.h"
#include "lib/debug.h"
#include "kernel/buddy_allocator.h"
#include "kernel/syscall_user.h"
#include "arch/x86/pit.h"

using namespace kernel;

#include "kernel/kernel.h"

extern "C" void timer_interrupt();
extern "C" void syscall_interrupt();
extern "C" void page_fault_interrupt();
extern "C" void general_protection_interrupt();
extern "C" void segmentation_fault_interrupt();

extern "C" void kernel_main() {
    // 初始化串口，用于调试输出
    serial_init();
    serial_puts("Hello, world!\n");

    // 初始化GDT
    GDT::init();
    serial_puts("GDT initialized!\n");

    // 初始化内核内存管理器
    serial_puts("Kernel memory manager initialized!\n");

    // 初始化中断管理器
    InterruptManager::init();
    serial_puts("InterruptManager initialized!\n");

    Kernel::init_all();
    Kernel *kernel = &Kernel::instance();
    kernel->init();
    // 注册系统调用处理函数
    SyscallManager::registerHandler(SYS_EXIT, exitHandler);
    SyscallManager::registerHandler(SYS_FORK, [](uint32_t a,
        uint32_t b, uint32_t c, uint32_t d)
        {
            debug_debug("fork syscall called!\n");
            return ProcessManager::fork();
        });

    // 注册时钟中断处理函数
    InterruptManager::registerHandler(0x20, []() {
        static volatile uint32_t tick = 0;
        tick++;
        if (tick > 5) {
            debug_debug("timer interrupt called!\n");
            Scheduler::timer_tick();
            tick = 0;
           ProcessManager::schedule();
         }
    });
    PIT::init();
    // 初始化IDT
    IDT::init();
    IDT::setGate(IRQ_TIMER, (uint32_t)timer_interrupt, 0x0B, 0x8E);
    IDT::setGate(INT_PAGE_FAULT, (uint32_t)page_fault_interrupt, 0x0B, 0x8E);
    IDT::setGate(INT_SYSCALL, (uint32_t)syscall_interrupt, 0x0B, 0x8E);
    IDT::setGate(INT_GP_FAULT, (uint32_t)general_protection_interrupt, 0x08, 0x8E);
    IDT::setGate(INT_SEGMENT_NP, (uint32_t)segmentation_fault_interrupt, 0x08, 0x8E);
    IDT::loadIDT();
    serial_puts("IDT initialized!\n");

    // 初始化控制台
    Console::init();
    serial_puts("Console initialized!\n");

    Console::setColor(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    Console::print("Welcome to Custom Kernel!\n");
    Console::setColor(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    Console::print("System initialization completed.\n");

    // 初始化进程管理器和调度器
    ProcessManager::init();
    Scheduler::init();
    Console::print("Process and Scheduler systems initialized!\n");

    // // // 创建并初始化第一个进程
    // uint32_t init_pid = ProcessManager::create_process("idle");
    // if (init_pid == 0) {
    //     debug_err("Failed to create init process!\n");
    //     return;
    // }
    // ProcessManager::switch_process(init_pid);
    // uint32_t cr3_val;
    // asm volatile("mov %%cr3, %0" : "=r" (cr3_val));
    // ProcessManager::get_current_process()->cr3 = cr3_val;
    // debug_debug("Init process created with pid %d, cr3: %x\n", init_pid, cr3_val);

    init_vfs();

    // 初始化内存文件系统
    debug_debug("Trying to new memfs ...\n");
    MemFS* memfs = new MemFS();
    debug_debug("memfs created at %x!\n", memfs);

    memfs->init();

    // int *p = (int *)0xC1000000;
    // *p = 0x12345678;

    debug_debug("memfs initialized!%x\n", memfs);
    debug_debug("memfs name:%s!\n", memfs->get_name());
    debug_debug("memfs initialized!\n");
    VFSManager::instance().register_fs("/", memfs);
    auto consolefs = new ConsoleFS();
    VFSManager::instance().register_fs("/dev/console", consolefs);
    debug_debug("memfs registered!\n");

    // 加载initramfs
    extern uint8_t __initramfs_start[];
    extern uint8_t __initramfs_end[];
    debug_info("initramfs start: %d\n", (unsigned int)__initramfs_start);
    debug_info("initramfs end: %d\n", (unsigned int)__initramfs_end);
    size_t initramfs_size = __initramfs_end - __initramfs_start;
    debug_info("initramfs size: %d\n", initramfs_size);

    char ll[]="123456";
    //char * data = (char*)__initramfs_start;
    char * data = (char*)ll;
    debug_info("initramfs data: %x\n", data[0]);
    debug_info("initramfs data: %x\n", data[1]);
    debug_info("initramfs data: %d\n", data[2]);
    debug_info("initramfs data: %d\n", data[3]);
    debug_info("initramfs data: %d\n", data[4]);
    debug_info("initramfs data: %d\n", data[5]);
    if(data[0]=='1') {
        debug_info("initramfs data0 is 1 \n");
    }
    data = (char*)__initramfs_start;
    if(data[0]==0x30) {
        debug_info("initramfs data0 is 0x30 \n");
    }
//    debug_info(data);
    int ret = memfs->load_initramfs(__initramfs_start, initramfs_size);
    if (ret < 0) {
        debug_alert("Failed to load initramfs!\n");
    }
    Console::print("MemFS initialized and initramfs loaded!\n");

    // 尝试加载并执行init程序
    auto cr3 = Kernel::instance().kernel_mm().paging().getCurrentPageDirectory();
    ProcessManager::get_current_process()->cr3 = Kernel::instance().kernel_mm().getPhysicalAddress(cr3);
    Console::print("Trying to execute /init...\n");
    ProcessManager::init_process(0, "idle");
    asm volatile("sti");
    int pid = syscall_fork();
    if (pid == 0) {
        // 子进程
        debug_debug("child process!\n");
        sys_execve((uint32_t)"/init", (uint32_t)nullptr, (uint32_t)nullptr);
        while (1) {
            debug_debug("child process!\n");
            asm volatile("hlt");
        }

    } else {
        // asm volatile("cli");
        while (1) {
            debug_debug("idle process!\n");
            asm volatile("hlt");
        }
    }
}
