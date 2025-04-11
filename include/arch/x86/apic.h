#pragma once

#include <cstdint>

namespace arch {

// LAPIC寄存器基址
#define LAPIC_BASE 0xFEE00000

// LAPIC寄存器偏移量
#define APIC_TIMER_VECTOR 0x30

// 传统PIC中断向量重映射 (IRQ0-15 -> 0x20-0x2F)


enum LapicRegisters {
    LAPIC_ID = 0x20,
    LAPIC_VER = 0x30,
    LAPIC_TPR = 0x80,
    LAPIC_EOI = 0xB0,
    LAPIC_SIVR = 0xF0,
    LAPIC_ICR0 = 0x300,
    LAPIC_ICR1 = 0x310
};

// IOAPIC寄存器
#define IOAPIC_BASE 0xFEC00000
#define IOAPIC_IOREGSEL 0x00
#define IOAPIC_IOWIN 0x10

// 中断命令寄存器格式
union icr_low {
    uint32_t raw;
    struct {
        uint32_t vector : 8;
        uint32_t delivery_mode : 3;
        uint32_t dest_mode : 1;
        uint32_t delivery_status : 1;
        uint32_t level : 1;
        uint32_t trigger_mode : 1;
        uint32_t dest_shorthand : 2;
        uint32_t reserved : 15;
    } __attribute__((packed));
};

void apic_init();
uint32_t apic_get_id();
void apic_send_eoi();
void apic_enable();
void apic_send_init(uint32_t target);
void apic_send_sipi(uint8_t vector, uint32_t target);
uint32_t apic_get_cpu_count();
void apic_init_timer(uint32_t hz);

// IOAPIC相关函数
void ioapic_init();
void ioapic_set_irq(uint8_t irq, uint64_t value);

} // namespace arch