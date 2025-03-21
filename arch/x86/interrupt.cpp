#include <cstdint>

#include "lib/ioport.h"
#include "arch/x86/interrupt.h"
#include "lib/debug.h"


void InterruptManager::init() {
    // 初始化ISR处理程序
    for (int i = 0; i < 32; i++) {
        isrHandlers[i] = defaultISRHandler;
    }

    // 初始化IRQ处理程序
    for (int i = 0; i < 16; i++) {
        irqHandlers[i] = defaultIRQHandler;
    }

    // 初始化系统调用处理
    SyscallManager::init();
    registerISRHandler(0x80, syscallHandler);

    // 重新映射PIC
    remapPIC();
}

void InterruptManager::registerISRHandler(uint8_t interrupt, ISRHandler handler) {
    debug_debug("registerISRHandler called with interrupt: %d\n", interrupt);
    isrHandlers[interrupt] = handler;
}

void InterruptManager::registerIRQHandler(uint8_t irq, IRQHandler handler) {
    irqHandlers[irq] = handler;
}

// ISR处理函数
void InterruptManager::handleISR(uint8_t interrupt) {

    debug_debug("handleISR called with interrupt: %d\n", interrupt);
    uint32_t syscall_num, arg1, arg2, arg3, arg4;
    asm volatile(
        "mov %%eax, %0\n"
        "mov %%ebx, %1\n"
        "mov %%ecx, %2\n"
        "mov %%edx, %3\n"
        "mov %%esi, %4\n"
        : "=m"(syscall_num), "=m"(arg1), "=m"(arg2), "=m"(arg3), "=m"(arg4)
    );
    debug_debug("syscall_num: %d, arg1:%d, arg2:%d, arg3:%d, arg4:%d\n", syscall_num, arg1, arg2, arg3, arg4);
    uintptr_t addr = *(uintptr_t*)(arg1);
    debug_debug("syscall_num: %d, arg1:%d\n", syscall_num, addr);



    if (isrHandlers[interrupt]) {
        isrHandlers[interrupt]();
    }
}

// IRQ处理函数
void InterruptManager::handleIRQ(uint8_t irq) {
    if (irqHandlers[irq]) {
        irqHandlers[irq]();
    }

    // 发送EOI信号
    if (irq >= 8) {
        outb(0xA0, 0x20); // 发送给从PIC
    }
    outb(0x20, 0x20); // 发送给主PIC
}


ISRHandler InterruptManager::isrHandlers[256];
IRQHandler InterruptManager::irqHandlers[16];

void InterruptManager::defaultISRHandler() {
    // 默认的ISR处理程序
}

// 系统调用处理程序
void InterruptManager::syscallHandler() {
    uint32_t syscall_num, arg1, arg2, arg3, arg4;
    asm volatile(
        "mov %%eax, %0\n"
        "mov %%ebx, %1\n"
        "mov %%ecx, %2\n"
        "mov %%edx, %3\n"
        "mov %%esi, %4\n"
        : "=m"(syscall_num), "=m"(arg1), "=m"(arg2), "=m"(arg3), "=m"(arg4)
    );
    debug_debug("syscall_num: %d, arg1:%d, arg2:%d, arg3:%d, arg4:%d\n", syscall_num, arg1, arg2, arg3, arg4);
    uintptr_t addr = *(uintptr_t*)(arg1);
    debug_debug("syscall_num: %d, arg1:%d\n", syscall_num, addr);
    debug_debug("syscall_num: %d, arg1:%s\n", syscall_num, *(char*)addr);

    // 调用系统调用处理器
    int ret = SyscallManager::handleSyscall(syscall_num, arg1, arg2, arg3, arg4);

    // 设置返回值
    asm volatile("mov %0, %%eax" : : "r"(ret));
}

void InterruptManager::defaultIRQHandler() {
    // 默认的IRQ处理程序
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