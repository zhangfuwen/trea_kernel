#ifndef ARCH_X86_INTERRUPT_H
#define ARCH_X86_INTERRUPT_H

#include <cstdint>
#include "kernel/syscall.h"

// 中断服务例程处理函数类型
typedef void (*ISRHandler)(void);

// 中断请求处理函数类型
typedef void (*IRQHandler)(void);


class InterruptManager {
public:
    static void init();

    // 注册中断处理函数
    static void registerISRHandler(uint8_t interrupt, ISRHandler handler);
    static void registerIRQHandler(uint8_t irq, IRQHandler handler);
    static void handleISR(uint8_t interrupt);
    static void handleIRQ(uint8_t irq);
    static ISRHandler isrHandlers[32];
    static IRQHandler irqHandlers[16];

    static void defaultISRHandler();

    // 系统调用处理程序
    static void syscallHandler();
    static void defaultIRQHandler();

    static void remapPIC();

};

#endif // ARCH_X86_INTERRUPT_H