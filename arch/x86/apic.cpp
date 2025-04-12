#include "arch/x86/apic.h"
#include "lib/debug.h"
#include "arch/x86/interrupt.h"

#include <lib/ioport.h>

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
    // 禁用传统8259A PIC
    outb(0x21, 0xFF);  // 屏蔽主PIC所有中断
    outb(0xA1, 0xFF);  // 屏蔽从PIC所有中断

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
    icr.delivery_mode = APIC_ICR_DELIVERY_INIT;
    icr.dest_mode = APIC_ICR_PHYSICAL_MODE;
    icr.level = APIC_ICR_LEVEL_ASSERT;
    icr.trigger_mode = APIC_ICR_TRIGGER_LEVEL;
    
    // 设置目标APIC ID
    apic_write(LAPIC_ICR1, target << APIC_ICR_DEST_SHIFT);
    
    // 发送IPI
    apic_write(LAPIC_ICR0, icr.raw);
    
    // 等待发送完成
    while (apic_read(LAPIC_ICR0) & APIC_ICR_PENDING_MASK) {
        asm volatile("pause");
    }
}

// 发送启动IPI (SIPI)
void apic_send_sipi(uint8_t vector, uint32_t target) {
    icr_low icr;
    icr.raw = 0;
    icr.vector = vector;
    icr.delivery_mode = APIC_ICR_DELIVERY_SIPI;
    icr.dest_mode = APIC_ICR_PHYSICAL_MODE;
    icr.level = APIC_ICR_LEVEL_ASSERT;
    icr.trigger_mode = APIC_ICR_TRIGGER_LEVEL;
    
    // 设置目标APIC ID
    apic_write(LAPIC_ICR1, target << APIC_ICR_DEST_SHIFT);
    
    // 发送IPI
}

void APICController::init_timer() {
    // 设置APIC Timer为周期模式
    apic_write(LAPIC_LVT_TIMER, APIC_TIMER_PERIODIC | APIC_TIMER_VECTOR);
    
    // 设置默认频率
    set_timer_frequency(DEFAULT_TIMER_FREQUENCY);
}

void APICController::set_timer_frequency(uint32_t frequency) {
    // 计算APIC Timer的分频值
    // 假设CPU频率为100MHz，要实现100Hz的中断频率，分频值应为1000000
    uint32_t cpu_freq_mhz = 100;  // CPU频率（MHz）
    uint32_t divisor = (cpu_freq_mhz * 1000000) / frequency;
    
    // 设置初始计数值
    apic_write(LAPIC_INITIAL_COUNT, divisor);
}

// 初始化APIC定时器
// 初始化APIC定时器，参数为期望的频率（Hz）
void apic_init_timer(uint32_t frequency) {
    // 假设APIC定时器的时钟频率为100MHz（可根据实际情况修改）
    const uint32_t apic_clock_frequency = 100000000;
    // 计算初始计数值
    uint32_t initial_count = apic_clock_frequency / (frequency * (APIC_TIMER_DIVIDE_16 + 1));

    // 配置APIC定时器为周期模式，向量号为0x40
    apic_write(LAPIC_LVT_TIMER, APIC_TIMER_VECTOR | APIC_TIMER_PERIODIC);

    // 设置初始计数值
    apic_write(LAPIC_INITIAL_COUNT, initial_count);

    // 设置除数为16
    apic_write(LAPIC_DIVIDE_CONFIG, APIC_TIMER_DIVIDE_16);
}

// 获取CPU数量
uint32_t apic_get_cpu_count() {
    // 在实际系统中，应该通过解析ACPI表获取CPU数量
    // 这里简单返回一个固定值用于测试
    return 4;
}

// 初始化IO APIC
void ioapic_init() {
    // 配置所有24个IRQ重定向
    const uint8_t vectors[] = {
        IRQ_TIMER,     // IRQ0
        IRQ_KEYBOARD,  // IRQ1
        IRQ_CASCADE,   // IRQ2
        IRQ_COM2,      // IRQ3
        IRQ_COM1,      // IRQ4
        IRQ_LPT2,      // IRQ5
        IRQ_FLOPPY,    // IRQ6
        IRQ_LPT1,      // IRQ7
        IRQ_RTC,       // IRQ8
        IRQ_PS2,       // IRQ9
        IRQ_RESV1,     // IRQ10
        IRQ_RESV2,     // IRQ11
        IRQ_PS2_AUX,   // IRQ12
        IRQ_FPU,       // IRQ13
        IRQ_ATA1,      // IRQ14
        IRQ_ATA2,      // IRQ15
        IRQ_ETH0,      // IRQ16
        IRQ_ETH1,      // IRQ17
        IRQ_USER_BASE, // IRQ18
        IRQ_USER_BASE+1, // IRQ19
        IRQ_USER_BASE+2, // IRQ20
        IRQ_USER_BASE+3, // IRQ21
        IRQ_USER_BASE+4, // IRQ22
        IRQ_IPI        // IRQ23
    };

    for (uint8_t i = 0; i < 24; i++) {
        uint64_t value = vectors[i] | (0 << 16); // 取消屏蔽
        value |= (1 << 14); // 电平触发模式
        value |= (apic_get_id() << 56); // 目标APIC ID
        ioapic_set_irq(i, value);
    }

    debug_debug("IO APIC initialized with all 24 IRQs configured\n");
}

// 设置IO APIC中断重定向表
void ioapic_set_irq(uint8_t irq, uint64_t value) {
    uint32_t ioredtbl = 0x10 + 2 * irq;
    
    // 写低32位
    ioapic_write(ioredtbl, (uint32_t)value);
    
    // 写高32位
    ioapic_write(ioredtbl + 1, (uint32_t)(value >> 32));
}

// APICController类实现
void APICController::init() {
    apic_init();
    ioapic_init();
    init_timer();
}

void APICController::send_eoi() {
    apic_send_eoi();
}

void APICController::enable_irq(uint8_t irq) {
    // 获取当前重定向表项的值
    uint32_t ioredtbl = 0x10 + 2 * irq;
    uint32_t low = ioapic_read(ioredtbl);
    uint32_t high = ioapic_read(ioredtbl + 1);
    
    // 清除屏蔽位
    low &= ~(1 << 16);
    
    // 写回重定向表项
    ioapic_write(ioredtbl, low);
    ioapic_write(ioredtbl + 1, high);
}

void APICController::disable_irq(uint8_t irq) {
    // 获取当前重定向表项的值
    uint32_t ioredtbl = 0x10 + 2 * irq;
    uint32_t low = ioapic_read(ioredtbl);
    uint32_t high = ioapic_read(ioredtbl + 1);
    
    // 设置屏蔽位
    low |= (1 << 16);
    
    // 写回重定向表项
    ioapic_write(ioredtbl, low);
    ioapic_write(ioredtbl + 1, high);
}

void APICController::remap_vectors() {
    // APIC不需要重映射向量
    // 在IOAPIC初始化时已经设置好了中断向量
}

uint32_t APICController::get_vector(uint8_t irq) const {
    // 根据不同IRQ返回对应的中断向量
    switch(irq) {
        case IRQ_TIMER:  // Timer
            return APIC_TIMER_VECTOR;  // 0x40
        // case 1:  // Keyboard
        //     return IRQ_KEYBOARD;       // 0x21
        // case 2:  // Cascade
        //     return IRQ_CASCADE;        // 0x22
        // case 8:  // RTC
        //     return IRQ_RTC;           // 0x28
        // case 14: // ATA1
        //     return IRQ_ATA1;          // 0x2E
        // case 15: // ATA2
        //     return IRQ_ATA2;          // 0x2F
        default:
            // 其他IRQ保持与PIC8259相同的映射
            return irq;
    }
}

} // namespace arch