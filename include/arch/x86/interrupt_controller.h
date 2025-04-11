#pragma once

#include <cstdint>

namespace arch {

// 中断控制器接口类
class InterruptController {
public:
    virtual ~InterruptController() = default;

    // 初始化中断控制器
    virtual void init() = 0;

    // 发送EOI信号
    virtual void send_eoi() = 0;

    // 启用指定中断
    virtual void enable_irq(uint8_t irq) = 0;

    // 禁用指定中断
    virtual void disable_irq(uint8_t irq) = 0;

    // 重映射中断向量
    virtual void remap_vectors() = 0;

    // 获取当前中断号
    virtual uint32_t get_current_interrupt() const = 0;

    // 将IRQ映射到中断向量
    virtual uint32_t get_vector(uint8_t irq) const = 0;

    // 初始化时钟
    virtual void init_timer() = 0;

    // 设置时钟频率
    virtual void set_timer_frequency(uint32_t frequency) = 0;

protected:
    // 存储当前中断号
    uint32_t current_interrupt = 0;

    // 时钟相关常量
    static constexpr uint32_t BASE_TIMER_FREQUENCY = 1193180;  // 基础时钟频率
    static constexpr uint32_t DEFAULT_TIMER_FREQUENCY = 100;   // 默认时钟频率（100Hz）
};

} // namespace arch