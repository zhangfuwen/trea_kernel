#include <cstdint>
#include "arch/x86/gdt.h"

// 定义TSS
TSSEntry GDT::tss;
GDTEntry GDT::entries[6];
GDTPointer GDT::gdtPointer;

extern "C" void loadGDT_ASM(GDTPointer*);
void GDT::init() {
    // 设置GDT表项
    setEntry(0, 0, 0, 0, 0);                // Null段
    setEntry(1, 0, 0xFFFFFFFF, GDT_PRESENT | GDT_DPL_0 | GDT_TYPE_CODE, 0xCF); // 内核代码段
    setEntry(2, 0, 0xFFFFFFFF, GDT_PRESENT | GDT_DPL_0 | GDT_TYPE_DATA, 0xCF); // 内核数据段
    setEntry(3, 0, 0xFFFFFFFF, GDT_PRESENT | GDT_DPL_3 | GDT_TYPE_CODE, 0xCF); // 用户代码段
    setEntry(4, 0, 0xFFFFFFFF, GDT_PRESENT | GDT_DPL_3 | GDT_TYPE_DATA, 0xCF); // 用户数据段

    // 初始化TSS
    tss.ss0 = 0x10;  // 内核数据段选择子
    tss.esp0 = 0;    // 将在进程创建时设置
    tss.iomap_base = sizeof(TSSEntry);

    // 设置TSS描述符
    uint32_t tss_base = reinterpret_cast<uint32_t>(&tss);
    setEntry(5, tss_base, sizeof(TSSEntry), GDT_PRESENT | GDT_TYPE_TSS, 0x00);

    // 加载GDT
    gdtPointer.limit = (sizeof(GDTEntry) * 6) - 1;
    gdtPointer.base = reinterpret_cast<uintptr_t>(&entries[0]);
    loadGDT();

    // 加载TSS
    asm volatile("ltr %%ax" : : "a"(0x28));  // TSS段选择子
}

void GDT::setEntry(int index, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    entries[index].base_low = base & 0xFFFF;
    entries[index].base_middle = (base >> 16) & 0xFF;
    entries[index].base_high = (base >> 24) & 0xFF;
    entries[index].limit_low = limit & 0xFFFF;
    entries[index].granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    entries[index].access = access;
}

void GDT::loadGDT() {
    loadGDT_ASM(&gdtPointer);
}