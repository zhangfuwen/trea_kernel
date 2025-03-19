#include <cstdint>

struct PageDirectory;
// 声明外部汇编函数
extern "C" uint32_t cr0_asm();
extern "C" uint32_t cr3_asm();

class PageManager {
public:
    static void init() {
        // 初始化页目录
        PageDirectory* pageDirectory = reinterpret_cast<PageDirectory*>(allocPage());
        for (int i = 0; i < 1024; i++) {
            pageDirectory->entries[i] = 0x00000002; // Supervisor, read/write, not present
        }

        // 映射前4MB的内存（内核空间）
        mapKernelSpace(pageDirectory);

        // 加载页目录
        loadPageDirectory(reinterpret_cast<uintptr_t>(pageDirectory));
        enablePaging();
    }

private:
    struct PageDirectory {
        uint32_t entries[1024];
    } __attribute__((aligned(4096)));

    struct PageTable {
        uint32_t entries[1024];
    } __attribute__((aligned(4096)));

    static uint32_t nextFreePage;

    static uint32_t allocPage() {
        uint32_t page = nextFreePage;
        nextFreePage += 4096;
        return page;
    }

    static void mapKernelSpace(PageDirectory* dir) {
        PageTable* table = reinterpret_cast<PageTable*>(allocPage());

        // 直接映射前4MB物理内存
        for (uint32_t i = 0; i < 1024; i++) {
            table->entries[i] = (i * 4096) | 3; // Supervisor, read/write, present
        }

        // 设置页目录项
        dir->entries[0] = reinterpret_cast<uintptr_t>(table) | 3; // Supervisor, read/write, present
    }


    static void loadPageDirectory(uint32_t dir) {
        asm volatile("mov cr3_asm, %0" : : "r"(dir));
    }

    static void enablePaging() {
        uint32_t cr0_val = cr0_asm();
        cr0_val |= 0x80000000; // 启用分页
        asm volatile("mov cr0_asm, %0" : : "r"(cr0_val));
    }
};

// 静态成员初始化
uint32_t PageManager::nextFreePage = 0x100000; // 从1MB开始分配页