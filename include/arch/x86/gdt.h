#ifndef ARCH_X86_GDT_H
#define ARCH_X86_GDT_H

#include "smp.h"

#include <cstdint>

struct TSSEntry {
    uint32_t prev_tss; // 前一个TSS的链接
    uint32_t esp0;     // 内核栈指针
    uint32_t ss0;      // 内核栈段选择子
    uint32_t esp1;
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;
    uint32_t cr3; // 页目录基址
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t es;
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;
    uint32_t ldt;
    uint16_t trap;
    uint16_t iomap_base;
} __attribute__((packed));

struct GDTEntry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed));

struct CallGateDescriptor {
    uint16_t offset_low;     // 目标代码偏移的低16位
    uint16_t selector;       // 目标代码段选择子
    uint8_t param_count : 5; // 参数数量(0-31)
    uint8_t reserved : 3;    // 保留位
    uint8_t type : 4;        // 类型(调用门为0xC)
    uint8_t s : 1;           // 必须为0(系统段)
    uint8_t dpl : 2;         // 描述符特权级
    uint8_t p : 1;           // 存在位
    uint16_t offset_high;    // 目标代码偏移的高16位
} __attribute__((packed));

#define GDT_PRESENT 0x80        // P位，表示段存在
#define GDT_DPL_0 0x00          // DPL为0，最高特权级
#define GDT_DPL_3 0x60          // DPL为3，最低特权级
#define GDT_TYPE_CODE 0x1A      // 代码段类型
#define GDT_TYPE_DATA 0x12      // 数据段类型
#define GDT_TYPE_TSS 0x09       // TSS类型
#define GDT_TYPE_CALL_GATE 0x0C // 调用门类型

struct GDTPointer {
    uint16_t limit;
    uintptr_t base;
} __attribute__((packed));

class GDT
{
public:
    static void init();

    static void updateTSS(uint32_t cpu, uint32_t esp0, uint32_t ss0);
    static void updateTSSCR3(uint32_t cpu, uint32_t cr3);
    static void setEntry(int index, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran);
    static void setCallGate(
        int index, uint16_t selector, uint32_t offset, uint8_t dpl, uint8_t param_count);
    static void loadGDT();
    static GDTEntry entries[6 + MAX_CPUS];
    static TSSEntry tss[MAX_CPUS];
    static GDTPointer gdtPointer;
};

#endif // ARCH_X86_GDT_H