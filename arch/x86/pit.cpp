#include <cstdint>
#include "lib/ioport.h"
#include "arch/x86/interrupt.h"

namespace PIT {
    // PIT端口定义
    const uint16_t PIT_CHANNEL0 = 0x40;    // 计数器0端口
    const uint16_t PIT_MODE = 0x43;        // 模式/命令端口
    const uint32_t PIT_FREQUENCY = 1193180; // PIT基准频率
//    const uint32_t DESIRED_FREQUENCY = 100; // 期望的时钟中断频率(Hz)
    const uint32_t DESIRED_FREQUENCY = 1; // 期望的时钟中断频率(Hz)

    // 初始化PIT
    void init() {
        // 计算分频值
        uint32_t divisor = PIT_FREQUENCY / DESIRED_FREQUENCY;
        
        // 设置计数器0，方式3（方波发生器）
        outb(PIT_MODE, 0x36);
        
        // 设置分频值（分两次写入，先低字节后高字节）
        outb(PIT_CHANNEL0, divisor & 0xFF);
        outb(PIT_CHANNEL0, (divisor >> 8) & 0xFF);
        outb(0x21, inb(0x21) & ~0x01); // 允许IRQ0

    }
}