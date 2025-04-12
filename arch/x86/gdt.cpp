#include "arch/x86/gdt.h"
#include <cstdint>

// 定义TSS
TSSEntry GDT::tss;
GDTEntry GDT::entries[7]; // 增加一个表项用于调用门
GDTPointer GDT::gdtPointer;

uint32_t gdt_entries = (uint32_t)GDT::entries;
// 用户态切换函数声明

extern "C" void loadGDT_ASM(GDTPointer*);
void GDT::init()
{
    // 设置GDT表项
    setEntry(0, 0, 0, 0, 0); // Null段
    setEntry(1, 0, 0xFFFFFFFF, GDT_PRESENT | GDT_DPL_0 | GDT_TYPE_CODE,
        0xCF); // 内核代码段
    setEntry(2, 0, 0xFFFFFFFF, GDT_PRESENT | GDT_DPL_0 | GDT_TYPE_DATA,
        0xCF); // 内核数据段
    setEntry(3, 0, 0xFFFFFFFF, GDT_PRESENT | GDT_DPL_3 | GDT_TYPE_CODE,
        0xCF); // 用户代码段
    setEntry(4, 0, 0xFFFFFFFF, GDT_PRESENT | GDT_DPL_3 | GDT_TYPE_DATA,
        0xCF); // 用户数据段

    // 初始化TSS
    tss.ss0 = 0x10; // 内核数据段选择子
    tss.esp0 = 0;   // 将在进程创建时设置
    tss.iomap_base = sizeof(TSSEntry);

    // 设置TSS描述符
    uint32_t tss_base = reinterpret_cast<uint32_t>(&tss);
    setEntry(5, tss_base, sizeof(TSSEntry), GDT_PRESENT | GDT_TYPE_TSS, 0x00);

    // 设置调用门描述符
    // 参数：索引、目标段选择子、目标偏移、特权级、参数数量
    // 目标段选择子为内核代码段(0x08)，特权级为3(允用户态调用)
    // uint32_t user_mode_entry_addr =
    // reinterpret_cast<uint32_t>(user_mode_entry); setCallGate(6, 0x08,
    // user_mode_entry_addr, 3, 0);

    // 加载GDT
    gdtPointer.limit = (sizeof(GDTEntry) * 7) - 1;
    gdtPointer.base = reinterpret_cast<uintptr_t>(&entries[0]);
    loadGDT();

    // 加载GDT后强制刷新CS
    asm volatile("ljmp $0x08, $1f\n" // 强制跳转到内核代码段
                 "1:\n"
                 "movw $0x10, %%ax\n" // 内核数据段选择子
                 "movw %%ax, %%ds\n"  // 更新DS
                 "movw %%ax, %%ss\n"  // 更新SS
                 :
                 :
                 : "ax");

    // 加载TSS
    asm volatile("ltr %%ax" : : "a"(0x28)); // TSS段选择子
}

void GDT::setEntry(int index, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran)
{
    entries[index].base_low = base & 0xFFFF;
    entries[index].base_middle = (base >> 16) & 0xFF;
    entries[index].base_high = (base >> 24) & 0xFF;
    entries[index].limit_low = limit & 0xFFFF;
    entries[index].granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    entries[index].access = access;
}

void GDT::setCallGate(
    int index, uint16_t selector, uint32_t offset, uint8_t dpl, uint8_t param_count)
{
    // 将GDTEntry结构重新解释为CallGateDescriptor
    CallGateDescriptor* gate = reinterpret_cast<CallGateDescriptor*>(&entries[index]);

    // 设置调用门描述符的各个字段
    gate->offset_low = offset & 0xFFFF;
    gate->offset_high = (offset >> 16) & 0xFFFF;
    gate->selector = selector;
    gate->param_count = param_count;
    gate->reserved = 0;
    gate->type = GDT_TYPE_CALL_GATE >> 4; // 调用门类型(0xC)
    gate->s = 0;                          // 系统段
    gate->dpl = dpl;                      // 描述符特权级
    gate->p = 1;                          // 存在位
}

void GDT::loadGDT()
{
    loadGDT_ASM(&gdtPointer);
}

// 更新TSS的内核栈指针
void GDT::updateTSS(uint32_t esp0, uint32_t ss0)
{
    tss.esp0 = esp0;
    tss.ss0 = ss0;
}

// 更新TSS的页目录基址
void GDT::updateTSSCR3(uint32_t cr3)
{
    tss.cr3 = cr3;
}

// 保存当前进程状态到TSS
// void GDT::saveTSSState(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t
// edx,
//                       uint32_t esi, uint32_t edi, uint32_t esp, uint32_t ebp,
//                       uint32_t eflags) {
//     tss.eax = eax;
//     tss.ebx = ebx;
//     tss.ecx = ecx;
//     tss.edx = edx;
//     tss.esi = esi;
//     tss.edi = edi;
//     tss.esp = esp;
//     tss.ebp = ebp;
//     tss.eflags = eflags;
// }