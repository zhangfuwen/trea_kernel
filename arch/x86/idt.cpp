#include <cstdint>

struct IDTEntry {
    uint16_t base_low;
    uint16_t selector;
    uint8_t zero;
    uint8_t flags;
    uint16_t base_high;
} __attribute__((packed));

struct IDTPointer {
    uint16_t limit;
    uintptr_t base;
} __attribute__((packed));

class IDT {
public:
    static void init() {
        // 初始化IDT表项
        for (int i = 0; i < 256; i++) {
            setGate(i, 0, 0x08, 0x8E); // 默认中断门
        }

        // 加载IDT
        idtPointer.limit = (sizeof(IDTEntry) * 256) - 1;
        idtPointer.base = reinterpret_cast<uintptr_t>(&entries[0]);
        loadIDT();
    }

    static void setGate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
        entries[num].base_low = base & 0xFFFF;
        entries[num].base_high = (base >> 16) & 0xFFFF;
        entries[num].selector = sel;
        entries[num].zero = 0;
        entries[num].flags = flags;
    }

private:
    static IDTEntry entries[256];
    static IDTPointer idtPointer;

    static void loadIDT() {
        asm volatile("lidt %0" : : "m"(idtPointer));
    }
};

IDTEntry IDT::entries[256];
IDTPointer IDT::idtPointer;