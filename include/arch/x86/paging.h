#ifndef ARCH_X86_PAGING_H
#define ARCH_X86_PAGING_H

#include <cstdint>

struct PageDirectory {
    uint32_t entries[1024];
} __attribute__((aligned(4096)));

struct PageTable {
    uint32_t entries[1024];
} __attribute__((aligned(4096)));

// 4M 以后开始分配内存
// 4M -> 4M + 4K 是PDT
// 4M + 4K -> 4M + 516K 是内存页表，为512K/4个条目，即128K*4K = 1024M = 1GB空间
// 以上是指物理内存，不需要虚拟地

constexpr uint32_t PAGE_DIRECTORY_ADDR = 0x400000; // 4MB地址处是页目录
constexpr uint32_t K_PAGE_TABLE_START = 0x401000;    // 4MB + 4KB地址处是页表
constexpr uint32_t K_PAGE_TABLE_COUNT =  224; // 224 * 1024page/table * 4KB/page = 896MB

class PageManager {
public:
    PageManager();
    void init();
    static void mapKernelSpace();
    static int copyMemorySpace(PageDirectory *src, PageDirectory* &out);

    static void loadPageDirectory(uint32_t dir);
    static void enablePaging();

    // 映射虚拟地址到物理地址
    void mapPage(uint32_t virt_addr, uint32_t phys_addr, uint32_t flags);
    void unmapPage(uint32_t virt_addr);

    // // 获取物理地址
    // uint32_t getPhysicalAddress(uint32_t virt_addr);

    // 获取当前页目录
    PageDirectory* getCurrentPageDirectory();
    PageTable * createPageTable();

    // 切换页目录
    void switchPageDirectory(PageDirectory* dir);

private:
    static void copyKernelSpace(PageDirectory * src, PageDirectory * dst);
    static int copyUserSpace(PageDirectory * src, PageDirectory * dst);
    uint32_t nextFreePage;
    PageDirectory* currentPageDirectory;
};

#endif // ARCH_X86_PAGING_H