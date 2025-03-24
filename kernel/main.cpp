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

// VGA 文本模式缓冲区地址
#define VGA_BUFFER 0xB8000
// VGA 文本模式的宽度和高度
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

extern "C" void timer_interrupt();
extern "C" void syscall_interrupt();

// 简单的字符输出函数
void print_char(char c) {
    static uint16_t *vga = (uint16_t *)VGA_BUFFER;
    static int x = 0;
    static int y = 0;

    if (c == '\n') {
        x = 0;
        y++;
        if (y >= VGA_HEIGHT) {
            y = 0;
        }
    } else {
        vga[y * VGA_WIDTH + x] = (uint16_t)c | 0x0F00;
        x++;
        if (x >= VGA_WIDTH) {
            x = 0;
            y++;
            if (y >= VGA_HEIGHT) {
                y = 0;
            }
        }
    }
}

void (*user_entry_point1)();
void user_entry_wrapper1() {
    // run user program
    debug_debug("user_entry_wrapper called with entry_point: %x\n", user_entry_point1);
    user_entry_point1();
    asm volatile("hlt");
    while (1) {
        static int i= 0;
        i++;
        if (i> 1000000) {
            i = 0;
            debug_debug("user_entry_wrapper ended with entry_point: %x\n", user_entry_point1);
        }
        // debug_debug("user_entry_wrapper ended with entry_point: %x\n", user_entry_point1);
    }
    syscall_exit(0);
}


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
            debug_debug("Timer interrupt!\n");
            Scheduler::timer_tick();
            tick = 0;
            //ProcessManager::schedule();
         }
    });
    PIT::init();
   // outb(0x20, 0x20); // 发送给主PIC
    // 初始化IDT
    IDT::init();
    IDT::setGate(0x80, (uint32_t)syscall_interrupt, 0x08, 0x8E);
    IDT::setGate(0x20, (uint32_t)timer_interrupt, 0x08, 0x8E);
    IDT::loadIDT();
    serial_puts("IDT initialized!\n");

    Kernel::init_all();
    Kernel &kernel = Kernel::instance();
    kernel.init();


    // asm volatile("hlt");
                // while (1) {
                //     // asm volatile("int $0x70");
                //     asm volatile("nop");
                //     asm volatile("nop");
                //     asm volatile("nop");
                //     asm volatile("nop");
                //     asm volatile("nop");
                //     asm volatile("nop");
                //     asm volatile("nop");
                //     asm volatile("nop");
                //     asm volatile("nop");
                //     asm volatile("nop");
                //     asm volatile("nop");
                //     // asm volatile("int $0x80");
                //     // debug_debug("wait!\n");
                //
                // }


    // 初始化控制台
//    Console::init();

//    print_char('H');
//    console_print("Welcome to Custom Kernel!\n");
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

    // 创建并初始化第一个进程
    uint32_t init_pid = ProcessManager::create_process("/init");
    if (init_pid == 0) {
        debug_err("Failed to create init process!\n");
        return;
    }
    ProcessManager::switch_process(init_pid);

    asm volatile("sti");
    int ret1 = syscall_fork();
    if (ret1 == 0) {
        // 子进程
        debug_debug("child process!\n");
        while (1) {
            // asm volatile("int $0x70");
            asm volatile("nop");
            asm volatile("nop");
            asm volatile("hlt");
            debug_debug("child process running!\n");
        }
    } else {
        debug_debug("child process returned %d\n", ret1);
        while (1) {
            // asm volatile("int $0x70");
            asm volatile("nop");
            asm volatile("nop");
            asm volatile("hlt");
            debug_debug("parent process running!\n");
        }
    }
    // ProcessManager::current_pid = init_pid;
    //
    // // 设置进程的页目录
    // uint32_t cr3_val;
    // asm volatile("mov %%cr3, %0" : "=r" (cr3_val));
    // ProcessManager::get_current_process()->cr3 = cr3_val;
    // debug_debug("Init process created with pid %d, cr3: %x\n", init_pid, cr3_val);

    init_vfs();

    // 初始化内存文件系统
    debug_debug("Trying to new memfs ...\n");
    MemFS* memfs = new MemFS();
    debug_debug("memfs created!\n");
    memfs->init();
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
    debug_info("initramfs data: %d\n", data[0]);
    debug_info("initramfs data: %d\n", data[1]);
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

#if 0
    FileDescriptor* fd = VFSManager::instance().open("/init");
    Console::print("Trying to open /init...\n");
    if (fd) {
        Console::print("File opened successfully!\n");
        // 读取ELF文件内容
        auto attr = new FileAttribute();
        int ret = VFSManager::instance().stat("/init", attr);
        debug_debug("File stat ret %d, size %d!\n",ret, attr->size);
        uint8_t* elf_data = new uint8_t[attr->size];
        ssize_t size = fd->read(elf_data, attr->size);
        debug_debug("File read successfully, size %d!\n", size);
        fd->close();

        if (size > 0) {
            // 加载并执行ELF文件
            debug_debug("Trying to load and execute init process...\n");
            uint32_t init_pid = ProcessManager::create_process("init");
            ProcessControlBlock* init_process = ProcessManager::get_current_process();
            debug_debug("Init process created, pid %d!\n", init_pid);
            ProcessManager::switch_process(init_pid);
            debug_debug("Init process switched!\n");

            if (init_pid <= 0) {
                debug_debug("Failed to create init process!\n");
                return;
            } else {
                debug_debug("Init process created!\n");
            }

            if (bool loaded = ElfLoader::load_elf(elf_data, size, 0x000000); loaded) {
                const ElfHeader* header = (const ElfHeader*)(elf_data);
                debug_debug("Init process will run at address %x\n", header->entry);

                // 分配内核栈
                uint8_t * stack = new uint8_t[4096];
                debug_debug("Stack allocated, running in kernel mode\n");

                // 设置进程的栈
                init_process->kernel_stack = (uint32_t)stack;
                init_process->esp0 = (uint32_t)stack + 4096 - 16; // 栈顶位置，预留一些空间

                // 直接在内核态执行程序
                uint32_t entry_point = header->entry + 0x000000; // 加上基址偏移
                debug_debug("Jumping to entry point at %x\n", entry_point);

                user_entry_point1 = (void (*)())entry_point;
                debug_debug("User entry point set to %x\n", user_entry_point1);
                asm volatile("hlt");
                ElfLoader::switch_to_user_mode((uint32_t)user_entry_wrapper1,(uint32_t)stack);
                debug_debug("Switched to user mode!\n");
                // 使用函数指针直接调用程序入口点
                // typedef void (*entry_func_t)();
                // entry_func_t entry_func = (entry_func_t)entry_point;
                // entry_func(); // 直接调用入口函数

                debug_debug("Init executed in kernel mode!\n");
            } else {
                debug_err("Init process loaded failed!\n");
            }
        }
        delete[] elf_data;
    } else {
        Console::print("Failed to open /init!\n");
    }
#else
    asm volatile("sti");
    // 尝试加载并执行init程序
    Console::print("Trying to execute /init...\n");
    // int32_t init_pid = ProcessManager::execute_process("/init");
    // if (init_pid < 0) {
        // Console::print("Failed to execute /init!\n");
        // return;
    // }
    debug_debug("Init process executed with pid: %d\n", init_pid);
#endif

    // 进入无限循环，等待中断
}