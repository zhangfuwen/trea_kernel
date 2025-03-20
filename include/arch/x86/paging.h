#ifndef ARCH_X86_PAGING_H
#define ARCH_X86_PAGING_H

#include <cstdint>

struct PageDirectory {
    uint32_t entries[1024];
} __attribute__((aligned(4096)));

struct PageTable {
    uint32_t entries[1024];
} __attribute__((aligned(4096)));



class PageManager {
public:
    static void init();
    static uint32_t allocPage();
    static void mapKernelSpace(PageDirectory* dir);
    static void loadPageDirectory(uint32_t dir);
    static void enablePaging();

private:
    static uint32_t nextFreePage;
    // static uint32_t* page_directory;
    // static uint32_t* page_tables;

};

#endif // ARCH_X86_PAGING_H