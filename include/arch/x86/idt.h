#ifndef ARCH_X86_IDT_H
#define ARCH_X86_IDT_H

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
    uint32_t base;
} __attribute__((packed));


class IDT {
public:
    static void init();

private:
    static IDTEntry entries[256];
    static IDTPointer idtPointer;

    static void setGate(uint8_t num, uint32_t base, uint16_t selector, uint8_t flags);
    static void loadIDT();
};

#endif // ARCH_X86_IDT_H