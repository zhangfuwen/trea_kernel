#pragma once

#include "arch/x86/interrupt_controller.h"
#include <cstdint>

// PIT相关定义
#define PIT_CHANNEL0 0x40     // 计数器0端口
#define PIT_MODE 0x43         // 模式/命令端口
#define PIT_FREQUENCY 1193180 // PIT基准频率
#define PIT_DESIRED_FREQUENCY 1 // 期望的时钟中断频率(Hz)

namespace arch {

// PIC命令和模式定义
#define PIC_ICW1_ICW4       0x01    // ICW4需要被发送
#define PIC_ICW1_INIT       0x10    // 初始化命令
#define PIC_ICW4_8086       0x01    // 8086/88模式
#define PIC_EOI             0x20    // 中断结束命令

// 中断向量偏移
#define PIC1_VECTOR_OFFSET  0x20    // 主PIC的IRQ0-7映射到0x20-0x27
#define PIC2_VECTOR_OFFSET  0x28    // 从PIC的IRQ8-15映射到0x28-0x2F

// 级联配置
#define PIC1_CASCADE_IRQ    0x04    // 从PIC连接到主PIC的IRQ2
#define PIC2_CASCADE_ID     0x02    // 从PIC的级联标识号

// PIC 8259A控制器实现
class PIC8259 : public InterruptController {
public:
    void init() override;
    void send_eoi() override;
    void enable_irq(uint8_t irq) override;
    void disable_irq(uint8_t irq) override;
    void remap_vectors() override;
    uint32_t get_current_interrupt() const override { return current_interrupt; }
    uint32_t get_vector(uint8_t irq) const override { return 0x20 + irq; } // IRQ0-15映射到0x20-0x2F

    // 实现时钟接口
    void init_timer() override;
    void set_timer_frequency(uint32_t frequency) override;

private:
    // PIT相关常量
    static constexpr uint16_t PIT_MODE = 0x43;
    static constexpr uint16_t PIT_CHANNEL0 = 0x40;

    static constexpr uint16_t PIC1_COMMAND = 0x20;
    static constexpr uint16_t PIC1_DATA = 0x21;
    static constexpr uint16_t PIC2_COMMAND = 0xA0;
    static constexpr uint16_t PIC2_DATA = 0xA1;
};

} // namespace arch