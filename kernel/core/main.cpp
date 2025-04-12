#include <arch/x86/apic.h>
#include <arch/x86/gdt.h>
#include <arch/x86/idt.h>
#include <arch/x86/interrupt.h>
#include <arch/x86/paging.h>
#include <arch/x86/smp.h>
#include <drivers/block_device.h>
#include <drivers/ext2.h>
#include <drivers/keyboard.h>
#include <kernel/buddy_allocator.h>
#include <kernel/elf_loader.h>
#include <kernel/memfs.h>
#include <kernel/process.h>
#include <kernel/scheduler.h>
#include <kernel/smp_scheduler.h>
#include <kernel/syscall_user.h>
#include <kernel/vfs.h>
#include <lib/console.h>
#include <lib/debug.h>
#include <lib/ioport.h>
#include <lib/serial.h>
#include <unistd.h>

using namespace kernel;

#include "kernel/kernel.h"

// 声明测试函数
void run_format_string_tests();

// extern "C" void apic_timer_interrupt();
extern "C" void timer_interrupt();
extern "C" void keyboard_interrupt();
extern "C" void cascade_interrupt();
extern "C" void ide1_interrupt();
extern "C" void ide2_interrupt();
extern "C" void syscall_interrupt();
extern "C" void page_fault_interrupt();
extern "C" void general_protection_interrupt();
extern "C" void segmentation_fault_interrupt();
extern "C" void stack_fault_interrupt();
ProcessControlBlock* init_proc = nullptr;
void init()
{
    debug_debug("entering init kernel code!\n");
    auto pcb = ProcessManager::get_current_process();
    debug_debug("pcb=0x%x\n", pcb);
    pcb->print();

    debug_debug("loading page directory 0x%x for pcb 0x%x(pid %d)\n", pcb->regs.cr3, pcb, pcb->pid);
    Kernel::instance().kernel_mm().paging().loadPageDirectory(pcb->regs.cr3);
    debug_debug("load page directory 0x%x for pcb 0x%x(pid %d)\n", pcb->regs.cr3, pcb, pcb->pid);
    while(true) {
        debug_rate_limited("init process!\n");
        sys_execve((uint32_t)"/init", (uint32_t)nullptr, (uint32_t)nullptr, init_proc);
        //    asm volatile("hlt");
    }
};

extern "C" void kernel_main()
{
    // 初始化串口，用于调试输出
    serial_init();
    serial_puts("Hello, world!\n");

    // 初始化GDT
    GDT::init();
    serial_puts("GDT initialized!\n");

    // 初始化内核内存管理器
    serial_puts("Kernel memory manager initialized!\n");

    // 初始化中断管理器

    Kernel::init_all();
    Kernel* kernel = &Kernel::instance();
    kernel->init();
    serial_puts("Kernel initialized!\n");

    kernel->interrupt_manager().init(InterruptManager::ControllerType::APIC);
    serial_puts("InterruptManager initialized!\n");

    // 运行格式化字符串测试
    run_format_string_tests();
    debug_debug("Kernel initialized!\n");
    // 注册系统调用处理函数
    SyscallManager::registerHandler(SYS_EXIT, exitHandler);
    SyscallManager::registerHandler(SYS_FORK, [](uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
        debug_debug("fork syscall called!\n");
        return ProcessManager::fork();
    });
    set_log_level(LOG_DEBUG);

    // 注册时钟中断处理函数
    kernel->interrupt_manager().registerHandler(0x20, []() {
        debug_rate_limited("timer interrupt 0x20!\n");
        Scheduler::timer_tick();
    });

    auto timer_interrupt_vector =
        kernel->interrupt_manager().get_controller()->get_vector(IRQ_TIMER);
    kernel->interrupt_manager().registerHandler(timer_interrupt_vector, []() {
        // debug_rate_limited("timer interrupt!\n");
            auto pcb = ProcessManager::get_current_process();
        if(pcb->debug_status & DEBUG_STATUS_HALT) {
            debug_debug("hlt timer interrupt\n");
            while(true) {
                asm volatile("hlt");
            }
        }
        static int count = 0;
        if(++count % 1000 == 0) {
            count = 0;
            //    debug_rate_limited("timer interrupt!\n");
            debug_debug("timer interrupt !\n");
            // Scheduler::timer_tick();
        }
    });
    debug_debug("timer interrupt vector: %d\n", timer_interrupt_vector);
    // 注册键盘中断处理函数
    keyboard_init();
    kernel->interrupt_manager().registerHandler(0x21, []() {
        debug_rate_limited("keyboard interrupt called!\n");
        auto pcb = ProcessManager::get_current_process();
        if(pcb->debug_status & DEBUG_STATUS_HALT) {
            debug_debug("hlt keyboard interrupt\n");
            while(true) {
                asm volatile("hlt");
            }
        }
        // auto code = keyboard_get_scancode();
        // auto ascii = scancode_to_ascii(code);
        // debug_debug("ascii 0x%x scancode 0x%x\n", ascii, code);
    });

    // 初始化IDT
    IDT::init();
    // 异常
    IDT::setGate(INT_PAGE_FAULT, (uint32_t)page_fault_interrupt, 0x08, 0xEE);
    IDT::setGate(INT_GP_FAULT, (uint32_t)general_protection_interrupt, 0x08, 0xEE);
    IDT::setGate(INT_SEGMENT_NP, (uint32_t)segmentation_fault_interrupt, 0x08, 0xEE);
    IDT::setGate(INT_STACK_FAULT, (uint32_t)stack_fault_interrupt, 0x08, 0xEE);
    // IDT::setGate(INT_COPROCESSOR_SEG, (uint32_t)stack_fault_interrupt, 0x08, 0xEE);

    IDT::setGate(timer_interrupt_vector, (uint32_t)timer_interrupt, 0x08, 0xEE);
    IDT::setGate(IRQ_KEYBOARD, (uint32_t)keyboard_interrupt, 0x08, 0xEE);
    IDT::setGate(IRQ_CASCADE, (uint32_t)cascade_interrupt, 0x08, 0xEE);
    IDT::setGate(IRQ_ATA1, (uint32_t)ide1_interrupt, 0x08, 0xEE);
    IDT::setGate(IRQ_ATA2, (uint32_t)ide2_interrupt, 0x08, 0xEE);
    // 软中断
    IDT::setGate(INT_SYSCALL, (uint32_t)syscall_interrupt, 0x08, 0xEE);
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
    // debug_debug("Init process created with pid %d, cr3: %x\n", init_pid,
    // cr3_val);

    init_vfs();

    // 初始化磁盘设备
    debug_debug("Initializing disk device...\n");
    auto disk = new DiskDevice();
    if(!disk->init()) {
        debug_err("Failed to initialize disk device!\n");
        return;
    }
    debug_debug("Disk device initialized!\n");

    // 初始化ext2文件系统
    debug_debug("Initializing ext2 filesystem...\n");
    auto ext2fs = new Ext2FileSystem(disk);
    VFSManager::instance().register_fs("/mnt", ext2fs);
    debug_debug("Ext2 filesystem mounted at /mnt\n");

    // 打印根目录内容
    auto root = VFSManager::instance().open("/mnt");
    if(root) {
        FileAttribute attr;
        VFSManager::instance().stat("/mnt/", &attr);
        debug_info("mnt directory size: %d bytes\n", attr.size);
        char buf[4096];
        ssize_t nread;
        struct dirent* dirp;

        // while ((nread = sys_getdents(fd, buf, sizeof(buf))) > 0) {
        //     for (char *b = buf; b < buf + nread;) {
        //         dirp = (struct dirent *)b;
        //         debug_debug("File: %s\n", dirp->d_name);
        //         b += dirp->d_reclen;
        //     }
        // }
        //
        // if (nread == -1) {
        //     perror("getdents");
        // }
        // ... 已有代码 ...
        delete root;
    } else {
        debug_err("Failed to open /mnt\n");
    }

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

    char ll[] = "123456";
    // char * data = (char*)__initramfs_start;
    char* data = (char*)ll;
    debug_info("initramfs data: %x\n", data[0]);
    debug_info("initramfs data: %x\n", data[1]);
    debug_info("initramfs data: %d\n", data[2]);
    debug_info("initramfs data: %d\n", data[3]);
    debug_info("initramfs data: %d\n", data[4]);
    debug_info("initramfs data: %d\n", data[5]);
    if(data[0] == '1') {
        debug_info("initramfs data0 is 1 \n");
    }
    data = (char*)__initramfs_start;
    if(data[0] == 0x30) {
        debug_info("initramfs data0 is 0x30 \n");
    }
    //    debug_info(data);
    int ret = memfs->load_initramfs(__initramfs_start, initramfs_size);
    if(ret < 0) {
        debug_alert("Failed to load initramfs!\n");
    }
    Console::print("MemFS initialized and initramfs loaded!\n");

    // 尝试加载并执行init程序
    auto cr3 = Kernel::instance().kernel_mm().paging().getCurrentPageDirectory();
    ProcessManager::get_current_process()->regs.cr3 = Kernel::instance().kernel_mm().virt2Phys(cr3);
    debug_info("Trying to execute /init...\n");
    ProcessManager::initIdle();
    debug_debug("Trying to execute /init...\n");
    init_proc = ProcessManager::kernel_thread("init", (uint32_t)init, 0, nullptr);
    debug_debug("init_proc: %x, pid:%d\n", init_proc, init_proc->pid);
    ProcessManager::cloneMemorySpace(init_proc, (ProcessControlBlock*)ProcessManager::idle_pcb);
    ProcessManager::allocUserStack(init_proc);
    init_proc->state = PROCESS_RUNNING;
    ProcessManager::appendPCB((PCB*)init_proc);

    debug_debug("init_proc: %x, pid:%d\n", init_proc, init_proc->pid);
    init_proc->print();
    debug_debug("init_proc initialized\n");
    
    // 初始化SMP，启动AP处理器
    arch::smp_init();
    debug_debug("SMP initialized\n");
    kernel->scheduler().init();
    kernel->scheduler().enqueue_task(init_proc);

    asm volatile("sti");
    while(1) {
        debug_rate_limited("idle process!\n");
        asm volatile("hlt");
        // debug_debug("x\n");
    }
}
