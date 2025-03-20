#include "arch/x86/gdt.h"
#include "arch/x86/idt.h"
#include "arch/x86/interrupt.h"
#include "arch/x86/paging.h"
#include "lib/console.h"
#include "lib/serial.h"
#include "process.h"
#include "scheduler.h"
#include "elf_loader.h"
#include "unistd.h"
#include "kernel/memfs.h"
#include "kernel/vfs.h"
#include "lib/debug.h"

using namespace kernel;

// VGA 文本模式缓冲区地址
#define VGA_BUFFER 0xB8000
// VGA 文本模式的宽度和高度
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

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

extern "C" void kernel_main() {
    serial_init();
    serial_puts("Hello, world!\n");
    // 初始化GDT
    GDT::init();
    serial_puts("GDT initialized!\n");

    // 初始化IDT
    IDT::init();
    serial_puts("IDT initialized!\n");

    // 初始化中断管理器
    InterruptManager::init();
    serial_puts("InterruptManager initialized!\n");


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

    // 初始化分页
    serial_puts("Paging initialization...\n");
    PageManager::init();
    Console::print("PageManager initialized!\n");
    serial_puts("PageManager initialized!\n");

    // 初始化进程管理器和调度器
    ProcessManager::init();
    Scheduler::init();
    Console::print("Process and Scheduler systems initialized!\n");

    // 初始化内存文件系统
    MemFS* memfs = new MemFS();
    memfs->init();
    VFSManager::instance().register_fs("/", memfs);

    // 加载initramfs
    extern uint8_t __initramfs_start[];
    extern uint8_t __initramfs_end[];
    size_t initramfs_size = __initramfs_end - __initramfs_start;
    int ret = memfs->load_initramfs(__initramfs_start, initramfs_size);
    if (ret < 0) {
        debug_alert("Failed to load initramfs!\n");
    }
    Console::print("MemFS initialized and initramfs loaded!\n");

    // 尝试加载并执行init程序
    FileDescriptor* fd = VFSManager::instance().open("/init");
    Console::print("Trying to open /init...\n");
    if (fd) {
        Console::print("File opened successfully!\n");
        // 读取ELF文件内容
        uint8_t* elf_data = new uint8_t[1024 * 1024]; // 假设最大1MB
        ssize_t size = fd->read(elf_data, 1024 * 1024);
        Console::print("File read successfully!\n");
        fd->close();

        if (size > 0) {
            // 加载并执行ELF文件
            uint32_t init_pid = ProcessManager::create_process("init");
            ProcessControlBlock* init_process = ProcessManager::get_current_process();
            if (init_pid > 0 && ElfLoader::load_elf(elf_data, size)) {
                const ElfHeader* header = (const ElfHeader*)(elf_data);
                Console::print("Init process loaded and started!\n");
                ElfLoader::execute(header->entry, init_process);
            }
        }
        delete[] elf_data;
    }

    // 进入无限循环，等待中断
    while (true) {
        asm volatile("hlt");
    }
}