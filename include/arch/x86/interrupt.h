#ifndef ARCH_X86_INTERRUPT_H
#define ARCH_X86_INTERRUPT_H

#include <cstdint>
#include "kernel/syscall.h"

// 处理器异常中断
#define INT_DIVIDE_ERROR       0x00    // 除零错误
#define INT_DEBUG              0x01    // 调试异常
#define INT_NMI                0x02    // 非屏蔽中断
#define INT_BREAKPOINT         0x03    // 断点
#define INT_OVERFLOW           0x04    // 溢出
#define INT_BOUND_RANGE        0x05    // 越界
#define INT_INVALID_OPCODE     0x06    // 无效操作码
#define INT_DEVICE_NA          0x07    // 设备不可用
#define INT_DOUBLE_FAULT       0x08    // 双重错误
#define INT_COPROCESSOR_SEG    0x09    // 协处理器段越界
#define INT_INVALID_TSS        0x0A    // 无效TSS
#define INT_SEGMENT_NP         0x0B    // 段不存在
#define INT_STACK_FAULT        0x0C    // 栈错误
#define INT_GP_FAULT           0x0D    // 通用保护错误
#define INT_PAGE_FAULT         0x0E    // 页错误
#define INT_RESERVED_0F        0x0F    // 保留
#define INT_FPU_FAULT          0x10    // x87浮点异常
#define INT_ALIGNMENT_CHECK    0x11    // 对齐检查
#define INT_MACHINE_CHECK      0x12    // 机器检查
#define INT_SIMD_FAULT         0x13    // SIMD浮点异常
#define INT_VIRT_EXCEPTION     0x14    // 虚拟化异常
#define INT_CONTROL_PROTECTION 0x15    // 控制保护异常
// 0x16-0x1F 保留

// 可编程中断控制器 (IRQ0-IRQ15)
#define IRQ_BASE               0x20    // 重映射后的IRQ基址
#define IRQ_TIMER              0x20    // 定时器中断
#define IRQ_KEYBOARD           0x21    // 键盘中断
#define IRQ_CASCADE            0x22    // PIC级联
#define IRQ_COM2               0x23    // 串口2
#define IRQ_COM1               0x24    // 串口1
#define IRQ_LPT2               0x25    // 并口2
#define IRQ_FLOPPY             0x26    // 软盘控制器
#define IRQ_LPT1               0x27    // 并口1
#define IRQ_CMOS               0x28    // CMOS时钟
#define IRQ_PS2_MOUSE          0x2C    // PS/2鼠标
#define IRQ_FPU                0x2D    // FPU异常
#define IRQ_ATA1               0x2E    // 主ATA控制器
#define IRQ_ATA2               0x2F    // 从ATA控制器

// 系统调用中断 (Linux传统值)
#define INT_SYSCALL            0x80    // 系统调用中断


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