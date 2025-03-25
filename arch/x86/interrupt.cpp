#include <cstdint>

#include "lib/ioport.h"
#include "arch/x86/interrupt.h"

#include <arch/x86/idt.h>

#include "lib/debug.h"
extern "C" void remap_pic();
extern "C" void enable_interrupts();
extern "C" void disable_interrupts();
extern "C" void init_idts();
extern "C" uint32_t timer_interrupt;
extern "C" uint32_t syscall_interrupt;


// 静态成员变量定义
InterruptHandler InterruptManager::handlers[256];



// ====== 初始化相关函数 ======

void InterruptManager::init() {
    // 初始化所有中断处理程序为默认处理程序
    for (int i = 0; i < 256; i++) {
        handlers[i] = defaultHandler;
    }

    // 初始化系统调用处理
    SyscallManager::init();
    // registerHandler(0x80, syscallHandler);

    // 注册各种中断处理程序
    registerHandler(0x21, []() { debug_debug("IRQ 1: Keyboard interrupt\n"); });
    registerHandler(0x22, []() { debug_debug("IRQ 2\n"); });
    registerHandler(0x23, []() { debug_debug("IRQ 3\n"); });
    registerHandler(0x24, []() { debug_debug("IRQ 4\n"); });
    registerHandler(0x25, []() { debug_debug("IRQ 5\n"); });
    registerHandler(0x26, []() {
        debug_debug("IRQ 6\n");
        uint8_t status = inb(0x3F4);
        if (status & 0x80) {
            debug_debug("IRQ 6: floppy is active\n");
        }
    });
    registerHandler(0x27, []() { debug_debug("IRQ 7\n"); });
    registerHandler(0x28, []() { debug_debug("IRQ 8\n"); });
    registerHandler(0x29, []() { debug_debug("IRQ 9\n"); });
    registerHandler(0x2A, []() { debug_debug("IRQ 10\n"); });
    registerHandler(0x2B, []() { debug_debug("IRQ 11\n"); });
    registerHandler(0x2C, []() { debug_debug("IRQ 12\n"); });
    registerHandler(0x2D, []() { debug_debug("IRQ 13\n"); });
    registerHandler(0x2E, []() { debug_debug("IRQ 14\n"); });
    registerHandler(0x2F, []() { debug_debug("IRQ 15\n"); });


    // IDT::setGate(0x80, (uint32_t)syscall_interrupt, 0x08, 0x8E);
    // IDT::setGate(0x20, (uint32_t)timer_interrupt, 0x08, 0x8E);
    // 重新映射PIC
    remapPIC();
}

// void InterruptManager::remapPIC() {
//     remap_pic();
// }

// ====== 中断处理核心函数 ======

extern "C" uint32_t handleSyscall(uint32_t syscall_num, uint32_t arg1, uint32_t arg2, uint32_t arg3)
{
    return SyscallManager::handleSyscall(syscall_num, arg1, arg2, arg3, 0);
}

extern "C" void handleInterrupt(uint32_t interrupt) {
    // debug_debug("[INT] Handling interrupt %d\n", interrupt);

    if (InterruptManager::handlers[interrupt]) {
        InterruptManager::handlers[interrupt]();
    } else {
        debug_debug("[INT] Unhandled interrupt %d\n", interrupt);
    }

    // 如果是IRQ，需要发送EOI
    if (interrupt >= 0x20 && interrupt <= 0x2F) {
        if (interrupt >= 0x28) {
            // 从PIC的中断
            outb(0xA0, 0x20);
        }
        outb(0x20, 0x20);
    }
}


// ====== 辅助函数 ======

void InterruptManager::registerHandler(uint8_t interrupt, InterruptHandler handler) {
    debug_debug("registerHandler called with interrupt: %d\n", interrupt);
    handlers[interrupt] = handler;
}

void InterruptManager::defaultHandler() {
    debug_debug("[INT] Unhandled interrupt\n");
}

void InterruptManager::enableInterrupts() {
    enable_interrupts();
}

void InterruptManager::disableInterrupts() {
    disable_interrupts();
}

void InterruptManager::remapPIC() {
    // 初始化主PIC
    outb(0x20, 0x11); // 初始化命令
    outb(0x21, 0x20); // 起始中断向量号
    outb(0x21, 0x04); // 告诉主PIC从PIC连接在IRQ2
    outb(0x21, 0x01); // 8086模式

    // 初始化从PIC
    outb(0xA0, 0x11); // 初始化命令
    outb(0xA1, 0x28); // 起始中断向量号
    outb(0xA1, 0x02); // 告诉从PIC连接到主PIC的IRQ2
    outb(0xA1, 0x01); // 8086模式

    // 清除PIC掩码
    outb(0x21, 0x0);
    outb(0xA1, 0x0);
}
