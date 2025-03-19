#include <cstdint>

// 中断服务例程处理函数类型
typedef void (*ISRHandler)(void);

// 中断请求处理函数类型
typedef void (*IRQHandler)(void);

class InterruptManager {
public:
    static void init() {
        // 初始化ISR处理程序
        for (int i = 0; i < 32; i++) {
            isrHandlers[i] = defaultISRHandler;
        }

        // 初始化IRQ处理程序
        for (int i = 0; i < 16; i++) {
            irqHandlers[i] = defaultIRQHandler;
        }

        // 重新映射PIC
        remapPIC();
    }

    static void registerISRHandler(uint8_t interrupt, ISRHandler handler) {
        isrHandlers[interrupt] = handler;
    }

    static void registerIRQHandler(uint8_t irq, IRQHandler handler) {
        irqHandlers[irq] = handler;
    }

    // ISR处理函数
    static void handleISR(uint8_t interrupt) {
        if (isrHandlers[interrupt]) {
            isrHandlers[interrupt]();
        }
    }

    // IRQ处理函数
    static void handleIRQ(uint8_t irq) {
        if (irqHandlers[irq]) {
            irqHandlers[irq]();
        }

        // 发送EOI信号
        if (irq >= 8) {
            outb(0xA0, 0x20); // 发送给从PIC
        }
        outb(0x20, 0x20); // 发送给主PIC
    }

private:
    static ISRHandler isrHandlers[32];
    static IRQHandler irqHandlers[16];

    static void defaultISRHandler() {
        // 默认的ISR处理程序
    }

    static void defaultIRQHandler() {
        // 默认的IRQ处理程序
    }

    static void remapPIC() {
        // 初始化主PIC
        outb(0x20, 0x11); // 初始化命令
        outb(0x21, 0x20); // 起始中断向量号
        outb(0x21, 0x04); // 告诉主PIC从PIC连接在IRQ2
        outb(0x21, 0x01); // 8086模式

        // 初始化从PIC
        outb(0xA0, 0x11); // 初始化命令
        outb(0xA1, 0x28); // 起始中断向量号
        outb(0xA1, 0x02); // 告诉从PIC连接到主PIC的IRQ2
        outb(0xA1, 0x01); // 8086模式

        // 清除PIC掩码
        outb(0x21, 0x0);
        outb(0xA1, 0x0);
    }

    static void outb(uint16_t port, uint8_t value) {
        asm volatile("outb %0, %1" : : "a"(value), "Nd"(port));
    }
};

// 静态成员初始化
ISRHandler InterruptManager::isrHandlers[32];
IRQHandler InterruptManager::irqHandlers[16];