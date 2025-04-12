#pragma once

#include "arch/x86/interrupt_controller.h"
#include <cstdint>

namespace arch {

// APIC基地址和寄存器偏移
#define LAPIC_BASE 0xFEE00000
#define IOAPIC_BASE 0xFEC00000

// IOAPIC寄存器偏移
#define IOAPIC_IOREGSEL 0x00
#define IOAPIC_IOWIN 0x10

// Local APIC寄存器偏移
#define LAPIC_ID 0x20
#define LAPIC_EOI 0xB0
#define LAPIC_SIVR 0xF0
#define LAPIC_ICR0 0x300
#define LAPIC_ICR1 0x310

// APIC定时器相关常量
#define APIC_TIMER_VECTOR 0x30
#define APIC_TIMER_PERIODIC 0x20000
#define APIC_TIMER_DIVIDE_16 0x3

// APIC寄存器定义
#define LAPIC_LVT_TIMER 0x320
#define LAPIC_INITIAL_COUNT 0x380
#define LAPIC_DIVIDE_CONFIG 0x3E0

// APIC ICR相关常量
#define APIC_ICR_DELIVERY_INIT 5
#define APIC_ICR_DELIVERY_SIPI 6
#define APIC_ICR_PHYSICAL_MODE 0
#define APIC_ICR_LEVEL_ASSERT 1
#define APIC_ICR_TRIGGER_LEVEL 1
#define APIC_ICR_PENDING_MASK (1 << 12)
#define APIC_ICR_DEST_SHIFT 24

// 中断向量号定义
#define IRQ_USER_BASE 0x32
#define IRQ_IPI 0x37

// MSR寄存器地址
#define MSR_APIC_BASE 0x1B
#define MSR_APIC_ENABLE_BIT 11

// PIC端口地址
#define PIC_MASTER_CMD 0x20
#define PIC_MASTER_DATA 0x21
#define PIC_SLAVE_CMD 0xA0
#define PIC_SLAVE_DATA 0xA1

// IOAPIC配置常量
#define IOAPIC_REDIR_TABLE_START 0x10
#define IOAPIC_LEVEL_TRIGGER (1 << 14)
#define IOAPIC_DEST_SHIFT 56

// ICR配置结构体
struct icr_low {
    union {
        struct {
            uint32_t vector : 8;
            uint32_t delivery_mode : 3;
            uint32_t dest_mode : 1;
            uint32_t delivery_status : 1;
            uint32_t reserved : 1;
            uint32_t level : 1;
            uint32_t trigger_mode : 1;
            uint32_t reserved2 : 2;
            uint32_t dest_shorthand : 2;
            uint32_t reserved3 : 12;
        };
        uint32_t raw;
    };
};

// APIC控制器实现
class APICController : public InterruptController {
public:
    void init() override;
    void send_eoi() override;
    void enable_irq(uint8_t irq) override;
    void disable_irq(uint8_t irq) override;
    void remap_vectors() override;
    uint32_t get_current_interrupt() const override { return current_interrupt; }
    uint32_t get_vector(uint8_t irq) const override;

    // 实现时钟接口
    void init_timer() override;
    void set_timer_frequency(uint32_t frequency) override;
};

// APIC函数声明
void apic_init();
void apic_init_timer(uint32_t frequency);
void apic_send_eoi();
void apic_enable();
void apic_send_init(uint32_t target);
void apic_send_sipi(uint8_t vector, uint32_t target);
uint32_t apic_get_id();
uint32_t apic_get_cpu_count();

// IOAPIC函数声明
void ioapic_init();
void ioapic_set_irq(uint8_t irq, uint64_t value);
void ioapic_enable_irq(uint8_t irq);
void ioapic_disable_irq(uint8_t irq);

} // namespace arch