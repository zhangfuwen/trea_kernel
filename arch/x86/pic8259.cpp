#include "arch/x86/pic8259.h"
#include "lib/ioport.h"
#include "lib/debug.h"

namespace arch {

void PIC8259::init() {
    // 初始化主从PIC
    outb(PIC1_COMMAND, PIC_ICW1_INIT | PIC_ICW1_ICW4);  // 初始化命令
    outb(PIC2_COMMAND, PIC_ICW1_INIT | PIC_ICW1_ICW4);
    
    // 重映射IRQ到中断向量
    outb(PIC1_DATA, PIC1_VECTOR_OFFSET);     // 主PIC的IRQ0-7映射到0x20-0x27
    outb(PIC2_DATA, PIC2_VECTOR_OFFSET);     // 从PIC的IRQ8-15映射到0x28-0x2F
    
    outb(PIC1_DATA, PIC1_CASCADE_IRQ);     // 告诉主PIC从PIC连接在IRQ2
    outb(PIC2_DATA, PIC2_CASCADE_ID);     // 告诉从PIC它的级联标识号
    
    outb(PIC1_DATA, PIC_ICW4_8086);     // 设置为x86模式
    outb(PIC2_DATA, PIC_ICW4_8086);
    
    // 屏蔽所有中断
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}

void PIC8259::send_eoi() {
    // 只有当中断号在0x20-0x2F范围内时才发送EOI
    uint32_t interrupt = get_current_interrupt();
    if (interrupt >= 0x20 && interrupt <= 0x2F) {
        // 发送EOI到主PIC
        outb(PIC1_COMMAND, PIC_EOI);
        // 如果是从PIC的中断（中断号大于等于0x28），也需要发送EOI到从PIC
        if (interrupt >= PIC2_VECTOR_OFFSET) {
            outb(PIC2_COMMAND, PIC_EOI);
        }
    }
}

void PIC8259::enable_irq(uint8_t irq) {
    uint16_t port;
    uint8_t value;
    
    if(irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }
    value = inb(port) & ~(1 << irq);
    outb(port, value);
}

void PIC8259::disable_irq(uint8_t irq) {
    uint16_t port;
    uint8_t value;
    
    if(irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }
    value = inb(port) | (1 << irq);
    outb(port, value);
}

void PIC8259::remap_vectors() {
    // 重映射IRQ到中断向量
    outb(PIC1_DATA, PIC1_VECTOR_OFFSET);     // 主PIC的IRQ0-7映射到0x20-0x27
    outb(PIC2_DATA, PIC2_VECTOR_OFFSET);     // 从PIC的IRQ8-15映射到0x28-0x2F
}

void PIC8259::init_timer() {
    // 设置计数器0，方式3（方波发生器）
    outb(PIT_MODE, 0x36);
    
    // 设置默认频率
    set_timer_frequency(DEFAULT_TIMER_FREQUENCY);
}

void PIC8259::set_timer_frequency(uint32_t frequency) {
    // 计算分频值
    uint32_t divisor = BASE_TIMER_FREQUENCY / frequency;

    // 设置分频值（分两次写入，先低字节后高字节）
    outb(PIT_CHANNEL0, divisor & 0xFF);
    outb(PIT_CHANNEL0, (divisor >> 8) & 0xFF);
}

} // namespace arch