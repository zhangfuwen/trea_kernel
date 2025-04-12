#include "arch/x86/interrupt.h"
#include "lib/debug.h"
#include <cstdint>

#include <arch/x86/apic.h>
#include <arch/x86/pic8259.h>
#include <kernel/kernel.h>
#include <lib/ioport.h>
extern "C" void remap_pic();
extern "C" void enable_interrupts();
extern "C" void disable_interrupts();
extern "C" void init_idts();
extern "C" uint32_t timer_interrupt;
extern "C" uint32_t syscall_interrupt;

// ====== 构造和析构函数 ======
InterruptManager::InterruptManager() : controller(nullptr)
{
    // 初始化所有中断处理程序为默认处理程序
    for(int i = 0; i < 256; i++) {
        handlers[i] = nullptr;
    }
}

InterruptManager::~InterruptManager()
{
    if(controller) {
        delete controller;
        controller = nullptr;
    }
}

// ====== 初始化相关函数 ======

void InterruptManager::init(ControllerType type)
{
    // 初始化所有中断处理程序为默认处理程序
    for(int i = 0; i < 256; i++) {
        handlers[i] = nullptr;
    }

    // 根据类型创建中断控制器
    if(type == ControllerType::PIC8259) {
        controller = new arch::PIC8259();
        debug_debug("Using PIC8259 interrupt controller\n");
    } else {
        controller = new arch::APICController();
        debug_debug("Using APIC interrupt controller\n");
    }

    // 初始化控制器
    controller->init();

    // 注册各种中断处理程序
    // registerHandler(IRQ_TIMER, []() { debug_debug("IRQ 0: Timer interrupt\n"); });
    // registerHandler(IRQ_KEYBOARD, []() { debug_debug("IRQ 1: Keyboard interrupt\n"); });
    // registerHandler(IRQ_COM2, []() { debug_debug("IRQ 3\n"); });
    // registerHandler(IRQ_COM1, []() { debug_debug("IRQ 4\n"); });
    // registerHandler(IRQ_LPT2, []() { debug_debug("IRQ 5\n"); });
    // registerHandler(IRQ_FLOPPY, []() {
    //     debug_debug("IRQ 6\n");
    //     uint8_t status = inb(0x3F4);
    //     if(status & 0x80) {
    //         debug_debug("IRQ 6: floppy is active\n");
    //     }
    // });
    // registerHandler(IRQ_LPT1, []() { debug_debug("IRQ 7\n"); });
    // registerHandler(IRQ_RTC, []() { debug_debug("IRQ 8\n"); });
    // registerHandler(IRQ_PS2, []() { debug_debug("IRQ 9\n"); });
    // registerHandler(IRQ_FPU, []() { debug_debug("IRQ 10\n"); });
    // registerHandler(IRQ_ATA1, []() { debug_debug("IRQ 11\n"); });
    // registerHandler(IRQ_ATA2, []() { debug_debug("IRQ 12\n"); });
    // registerHandler(IRQ_ETH0, []() { debug_debug("IRQ 13\n"); });
    // registerHandler(IRQ_ETH1, []() { debug_debug("IRQ 14\n"); });
    // registerHandler(IRQ_IPI, []() { debug_debug("IRQ 15\n"); });

    debug_debug("Interrupt controller initialization completed\n");
}

// void InterruptManager::remapPIC() {
//     remap_pic();
// }

// ====== 中断处理核心函数 ======

// 声明中断号到中断名称的映射表
extern const char* interrupt_names[256];

extern "C" void handleInterrupt(uint32_t interrupt)
{
    auto& im = Kernel::instance().interrupt_manager();
    // 设置当前中断号
    if(im.get_controller()) {
        im.get_controller()->current_interrupt = interrupt;
    }

    if(im.handlers[interrupt]) {
        im.handlers[interrupt]();
    } else {
        debug_debug("[INT] Unhandled interrupt %d(0x%x), name:%s\n", interrupt, interrupt,
            interrupt_names[interrupt]);
    }

    // 发送EOI（由具体的中断控制器决定是否需要发送）
    im.sendEOI();
}

// ====== 辅助函数 ======

void InterruptManager::registerHandler(uint8_t interrupt, InterruptHandler handler)
{
    debug_debug("registerHandler called with interrupt: %d\n", interrupt);
    handlers[interrupt] = handler;
}

void InterruptManager::enableInterrupts() { enable_interrupts(); }

void InterruptManager::disableInterrupts() { disable_interrupts(); }

void InterruptManager::sendEOI()
{
    if(controller) {
        controller->send_eoi();
    }
}

void InterruptManager::enableIRQ(uint8_t irq)
{
    if(controller) {
        controller->enable_irq(irq);
    }
}

void InterruptManager::disableIRQ(uint8_t irq)
{
    if(controller) {
        controller->disable_irq(irq);
    }
}
