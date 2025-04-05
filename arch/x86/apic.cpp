#include "arch/x86/apic.h"
#include <lib/ioport.h>
#include <lib/debug.h>

namespace arch {

// APIC寄存器访问函数
static inline uint32_t apic_read(uint32_t reg) {
    return *((volatile uint32_t*)(LAPIC_BASE + reg));
}

static inline void apic_write(uint32_t reg, uint32_t value) {
    *((volatile uint32_t*)(LAPIC_BASE + reg)) = value;
}

// IOAPIC寄存器访问函数
static inline void ioapic_write(uint32_t reg, uint32_t value) {
    *((volatile uint32_t*)(IOAPIC_BASE + IOAPIC_IOREGSEL)) = reg;
    *((volatile uint32_t*)(IOAPIC_BASE + IOAPIC_IOWIN)) = value;
}

static inline uint32_t ioapic_read(uint32_t reg) {
    *((volatile uint32_t*)(IOAPIC_BASE + IOAPIC_IOREGSEL)) = reg;
    return *((volatile uint32_t*)(IOAPIC_BASE + IOAPIC_IOWIN));
}

// 初始化本地APIC
void apic_init() {
    // 启用APIC
    uint32_t low, high;
    
    // 读取MSR 0x1B (APIC Base Address)
    asm volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(0x1B));
    
    // 设置APIC启用位 (bit 11)
    low |= (1 << 11);
    
    // 写回MSR
    asm volatile("wrmsr" : : "a"(low), "d"(high), "c"(0x1B));
    
    // 配置Spurious Interrupt Vector Register
    apic_write(LAPIC_SIVR, 0x100 | 0xFF);
    
    // 初始化APIC定时器
    apic_init_timer();
}

// 获取本地APIC ID
uint32_t apic_get_id() {
    return (apic_read(LAPIC_ID) >> 24) & 0xFF;
}

// 发送EOI信号
void apic_send_eoi() {
    apic_write(LAPIC_EOI, 0);
}

// 启用本地APIC
void apic_enable() {
    apic_write(LAPIC_SIVR, apic_read(LAPIC_SIVR) | 0x100);
}

// 发送INIT IPI
void apic_send_init(uint32_t target) {
    icr_low icr;
    icr.raw = 0;
    icr.vector = 0;
    icr.delivery_mode = 5; // INIT
    icr.dest_mode = 0;     // 物理目标模式
    icr.level = 1;
    icr.trigger_mode = 1;  // 电平触发
    
    // 设置目标APIC ID
    apic_write(LAPIC_ICR1, target << 24);
    
    // 发送IPI
    apic_write(LAPIC_ICR0, icr.raw);
    
    // 等待发送完成
    while (apic_read(LAPIC_ICR0) & (1 << 12)) {
        asm volatile("pause");
    }
}

// 发送启动IPI (SIPI)
void apic_send_sipi(uint8_t vector, uint32_t target) {
    icr_low icr;
    icr.raw = 0;
    icr.vector = vector;
    icr.delivery_mode = 6; // 启动IPI
    icr.dest_mode = 0;     // 物理目标模式
    icr.level = 1;
    icr.trigger_mode = 1;  // 电平触发
    
    // 设置目标APIC ID
    apic_write(LAPIC_ICR1, target << 24);
    
    // 发送IPI
    apic_write(LAPIC_ICR0, icr.raw);
    
    // 等待发送完成
    while (apic_read(LAPIC_ICR0) & (1 << 12)) {
        asm volatile("pause");
    }
}

// 初始化APIC定时器
void apic_init_timer() {
    // 配置APIC定时器为周期模式，向量号为0x40
    apic_write(0x320, 0x40 | 0x20000); // LVT Timer Register
    
    // 设置初始计数值
    apic_write(0x380, 10000000); // Initial Count Register
    
    // 设置除数为16
    apic_write(0x3E0, 0x3); // Divide Configuration Register
}

// 获取CPU数量
uint32_t apic_get_cpu_count() {
    // 在实际系统中，应该通过解析ACPI表获取CPU数量
    // 这里简单返回一个固定值用于测试
    return 4;
}

// 初始化IO APIC
void ioapic_init() {
    // 重定向所有中断到BSP
    for (uint8_t i = 0; i < 24; i++) {
        ioapic_set_irq(i, 0x10000); // 禁用所有中断
    }
    
    // 配置键盘中断 (IRQ 1)
    ioapic_set_irq(1, 0x21 | (apic_get_id() << 56));
    
    // 配置时钟中断 (IRQ 0)
    ioapic_set_irq(0, 0x20 | (apic_get_id() << 56));
    
    // 配置ATA控制器中断
    ioapic_set_irq(14, 0x2E | (apic_get_id() << 56)); // 主ATA控制器 (IRQ 14 -> 向量 0x2E)
    ioapic_set_irq(15, 0x2F | (apic_get_id() << 56)); // 从ATA控制器 (IRQ 15 -> 向量 0x2F)
    
    // 配置中断触发模式和掩码位
    // 低32位格式: 向量号(0-7) | 传递模式(8-10) | 目标模式(11) | 0(12) | 中断极性(13) | 触发模式(14) | 掩码位(16)
    // 高32位格式: 目标APIC ID(56-63)
    
    debug_debug("IO APIC initialized with %d interrupts configured\n", 4);
}

// 设置IO APIC中断重定向表
void ioapic_set_irq(uint8_t irq, uint64_t value) {
    uint32_t ioredtbl = 0x10 + 2 * irq;
    
    // 写低32位
    ioapic_write(ioredtbl, (uint32_t)value);
    
    // 写高32位
    ioapic_write(ioredtbl + 1, (uint32_t)(value >> 32));
}

} // namespace arch