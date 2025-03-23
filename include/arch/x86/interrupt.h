#ifndef ARCH_X86_INTERRUPT_H
#define ARCH_X86_INTERRUPT_H

#include <cstdint>
#include "kernel/syscall.h"

// 统一的中断处理函数类型
typedef void (*InterruptHandler)(void);

// 声明为C风格函数，以便汇编代码调用
extern "C" {
    void handleInterrupt(uint32_t interrupt);
}

class InterruptManager {
public:
    static void init();

    // 注册中断处理函数
    static void registerHandler(uint8_t interrupt, InterruptHandler handler);
    static InterruptHandler handlers[256];

    static void defaultHandler();

    // 系统调用处理程序
    static void syscallHandler();

    static void remapPIC();

    // 中断控制函数
    static void enableInterrupts();
    static void disableInterrupts();
};

#endif // ARCH_X86_INTERRUPT_H