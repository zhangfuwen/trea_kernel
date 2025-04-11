#include <cstdint>

#include "arch/x86/interrupt.h"
#include "lib/ioport.h"

#include <arch/x86/apic.h>

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

void InterruptManager::init()
{
    // 初始化所有中断处理程序为默认处理程序
    for(int i = 0; i < 256; i++) {
        handlers[i] = defaultHandler;
    }

    // 注册各种中断处理程序
    registerHandler(IRQ_TIMER, []() { debug_debug("IRQ 0: Timer interrupt\n"); });
    registerHandler(IRQ_KEYBOARD, []() { debug_debug("IRQ 1: Keyboard interrupt\n"); });
    registerHandler(IRQ_CASCADE, []() { debug_debug("IRQ 2\n"); });
    registerHandler(IRQ_COM2, []() { debug_debug("IRQ 3\n"); });
    registerHandler(IRQ_COM1, []() { debug_debug("IRQ 4\n"); });
    registerHandler(IRQ_LPT2, []() { debug_debug("IRQ 5\n"); });
    registerHandler(IRQ_FLOPPY, []() {
        debug_debug("IRQ 6\n");
        uint8_t status = inb(0x3F4);
        if(status & 0x80) {
            debug_debug("IRQ 6: floppy is active\n");
        }
    });
    registerHandler(IRQ_LPT1, []() { debug_debug("IRQ 7\n"); });
    registerHandler(IRQ_RTC, []() { debug_debug("IRQ 8\n"); });
    registerHandler(IRQ_PS2, []() { debug_debug("IRQ 9\n"); });
    registerHandler(IRQ_FPU, []() { debug_debug("IRQ 10\n"); });
    registerHandler(IRQ_ATA1, []() { debug_debug("IRQ 11\n"); });
    registerHandler(IRQ_ATA2, []() { debug_debug("IRQ 12\n"); });
    registerHandler(IRQ_ETH0, []() { debug_debug("IRQ 13\n"); });
    registerHandler(IRQ_ETH1, []() { debug_debug("IRQ 14\n"); });
    registerHandler(IRQ_IPI, []() { debug_debug("IRQ 15\n"); });

    // 初始化APIC而不是PIC
    arch::apic_init();
    arch::ioapic_init();
    
    // 禁用传统PIC控制器
    outb(0xA1, 0xFF);
    outb(0x21, 0xFF);
    
    // 注册APIC中断处理程序
    registerHandler(APIC_TIMER_VECTOR, []() { debug_debug("APIC Timer interrupt\n"); });
    registerHandler(0x22, []() { debug_debug("IRQ 2\n"); });
    registerHandler(0x23, []() { debug_debug("IRQ 3\n"); });
    registerHandler(0x24, []() { debug_debug("IRQ 4\n"); });
    registerHandler(0x25, []() { debug_debug("IRQ 5\n"); });
    registerHandler(0x26, []() {
        debug_debug("IRQ 6\n");
        uint8_t status = inb(0x3F4);
        if(status & 0x80) {
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

    // 初始化APIC而不是PIC
    // 不再需要重映射PIC
    debug_debug("APIC initialization will be done in kernel_main\n");
}

// void InterruptManager::remapPIC() {
//     remap_pic();
// }

// ====== 中断处理核心函数 ======


extern "C" void handleInterrupt(uint32_t interrupt)
{
    // debug_debug("[INT] Handling interrupt %d\n", interrupt);

    if(InterruptManager::handlers[interrupt]) {
        InterruptManager::handlers[interrupt]();
    } else {
        debug_debug("[INT] Unhandled interrupt %d\n", interrupt);
    }

    // 如果是IRQ，需要发送EOI到APIC
    if(interrupt >= 0x20 && interrupt <= 0x2F) {
        // 使用APIC发送EOI
        arch::apic_send_eoi();
        
        // 对于多核系统，可能需要处理处理器间中断
        if (interrupt == 0x20) { // 时钟中断
            // 在这里可以添加多核调度相关代码
        }
    }
}

// ====== 辅助函数 ======

void InterruptManager::registerHandler(uint8_t interrupt, InterruptHandler handler)
{
    debug_debug("registerHandler called with interrupt: %d\n", interrupt);
    handlers[interrupt] = handler;
}

void InterruptManager::defaultHandler()
{
    debug_debug("[INT] Unhandled interrupt\n");
}

void InterruptManager::enableInterrupts()
{
    enable_interrupts();
}

void InterruptManager::disableInterrupts()
{
    disable_interrupts();
}

// 此函数已弃用，系统现在使用APIC而不是PIC
// 保留此函数仅用于兼容性目的，新代码应该使用arch::apic_init()和arch::ioapic_init()
void InterruptManager::remapPIC()
{
    // 此函数保留为空，以保持兼容性
    // 实际上我们使用APIC而不是PIC
    debug_debug("警告：remapPIC函数已弃用，系统使用APIC而不是PIC，请使用arch::apic_init()代替\n");
}
