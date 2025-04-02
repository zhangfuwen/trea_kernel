#include "arch/x86/idt.h"
#include <cstdint>

extern "C" uint32_t timer_interrupt;
extern "C" uint32_t syscall_interrupt;

void IDT::init()
{
    // 初始化IDT表项
    for(int i = 0; i < 256; i++) {
        IDT::setGate(i, 0, 0x08, 0x8E); // 默认中断门
    }

    // 加载IDT
    IDT::idtPointer.limit = (sizeof(IDTEntry) * 256) - 1;
    IDT::idtPointer.base = reinterpret_cast<uintptr_t>(&IDT::entries[0]);


    IDT::loadIDT();
}

void IDT::setGate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags)
{
    IDT::entries[num].base_low = base & 0xFFFF;
    IDT::entries[num].base_high = (base >> 16) & 0xFFFF;
    IDT::entries[num].selector = sel;
    IDT::entries[num].zero = 0;
    IDT::entries[num].flags = flags;
}

void IDT::loadIDT()
{
    asm volatile("lidt %0" : : "m"(IDT::idtPointer));
}

IDTEntry IDT::entries[256];
IDTPointer IDT::idtPointer;