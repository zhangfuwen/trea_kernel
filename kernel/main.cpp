#include "../arch/x86/gdt.cpp"
#include "../arch/x86/idt.cpp"
#include "../arch/x86/interrupt.cpp"
#include "../arch/x86/paging.cpp"
#include "lib/console.h"
#include "lib/serial.h"

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

    // 进入无限循环，等待中断
    while (true) {
        asm volatile("hlt");
    }
}