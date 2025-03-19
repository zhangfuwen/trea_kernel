#include <cstdint>

struct GDTEntry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed));

struct GDTPointer {
    uint16_t limit;
    uintptr_t base;
} __attribute__((packed));
extern "C" void loadGDT_ASM(GDTPointer*);
class GDT {
public:
    static void init() {
        // 设置GDT表项
        setEntry(0, 0, 0, 0, 0);                // Null段
        setEntry(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // 代码段
        setEntry(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // 数据段

        // 加载GDT
        gdtPointer.limit = (sizeof(GDTEntry) * 3) - 1;
        gdtPointer.base = reinterpret_cast<uintptr_t>(&entries[0]);
        loadGDT();
    }

private:
    static GDTEntry entries[3];
    static GDTPointer gdtPointer;

    static void setEntry(int index, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
        entries[index].base_low = base & 0xFFFF;
        entries[index].base_middle = (base >> 16) & 0xFF;
        entries[index].base_high = (base >> 24) & 0xFF;
        entries[index].limit_low = limit & 0xFFFF;
        entries[index].granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);
        entries[index].access = access;
    }

    static void loadGDT() {
        loadGDT_ASM(&gdtPointer);
    }
};

GDTEntry GDT::entries[3];
GDTPointer GDT::gdtPointer;