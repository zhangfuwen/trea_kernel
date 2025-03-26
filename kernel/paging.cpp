#include <cstdint>
#include "../include/lib/console.h"
#include "../include/arch/x86/paging.h"

#include <kernel/page.h>
#include <lib/serial.h>

#include "kernel/kernel.h"

PageManager::PageManager() : nextFreePage(0x400000), currentPageDirectory(nullptr) {}

void PageManager::init() {
    // 初始化页目录
    serial_puts("PageManager: enter init\n");
    PageDirectory* pageDirectory = reinterpret_cast<PageDirectory*>(PAGE_DIRECTORY_ADDR);
    for (int i = 0; i < 1024; i++) {
        pageDirectory->entries[i] = 0x00000002; // Supervisor, read/write, not present
    }
    serial_puts("PageManager: pageDirectory init\n");

    mapKernelSpace();
    serial_puts("PageManager: kernel space mapped\n");

    // 加载页目录
    loadPageDirectory(reinterpret_cast<uintptr_t>(PAGE_DIRECTORY_ADDR));
    serial_puts("PageManager: pageDirectory load\n");
    enablePaging();
    serial_puts("PageManager: enable paging\n");
    // int *p = (int *)0xC1000000;
    // *p = 0x12345678;

    currentPageDirectory = (PageDirectory*)PAGE_DIRECTORY_ADDR + KERNEL_DIRECT_MAP_START; // 虚拟地址
}

// 映射内核空间 896MB
void PageManager::mapKernelSpace() {
    // 当前使用物理地址，实模式
    // 映射前4M
    auto *dir = reinterpret_cast<PageDirectory*>(PAGE_DIRECTORY_ADDR);
    auto *table1 = reinterpret_cast<PageTable*>(K_FIRST_4M_PT);
    dir->entries[0] = K_FIRST_4M_PT | 3; // Supervisor, read/write, present;
    for (int i = 0; i < 1024; i++) {
        table1->entries[i] = (i*4096) | 3; // Supervisor, read/write, present
    }
    table1++;
    dir->entries[1] = (uint32_t)table1 | 3; // Supervisor, read/write, present;
    for (int i = 0; i < 1024; i++) {
        table1->entries[i] = (i*4096 + 1024*4096) | 3; // Supervisor, read/write, present
    }

    // map 0xC0000000
    uint32_t pteStart = 0xC0000000 >> 22;
    for(int j = 0; j < K_PAGE_TABLE_COUNT; j++) {
        auto* table = reinterpret_cast<PageTable*>(K_PAGE_TABLE_START + j*sizeof(PageTable));
        for (uint32_t i = 0; i < 1024; i++) {
            table->entries[i] = (i * 4096 + j * 1024*4096) | 3; // Supervisor, read/write, present
        }
        dir->entries[j+pteStart] = reinterpret_cast<uintptr_t>(table) | 3; // Supervisor, read/write, present
    }
}


void PageManager::copyKernelSpace(PageDirectory * src, PageDirectory * dst) {
    // init
    for (int i = 0; i < 1024; i++) {
        dst->entries[i] = 0x00000002; // Supervisor, read/write, not present
    }
    // copy
    dst->entries[0] = src->entries[0];
    for (int i = 0; i < K_PAGE_TABLE_COUNT; i++) {
        dst->entries[i] = src->entries[i];
    }
}

int PageManager::copyUserSpace(PageDirectory * src, PageDirectory * dst) {
    int used_pt = 0;
    // 从K_PAGE_TABLE_COUNT开始，因为前面是内核空间
    for (uint32_t i = K_PAGE_TABLE_COUNT; i < 1024; i++) {
        // 如果源页目录项存在
        if (src->entries[i] & 0x1) {

            // 获取源页表
            PageTable* src_pt = reinterpret_cast<PageTable*>(src->entries[i] & 0xFFFFF000);
            auto page = Kernel::instance().kernel_mm().alloc_pages(1);
            if (page == nullptr) {
                Console::print("copyUserSpace: alloc_pages failed");
                return -1;
            }
            PageTable* dst_pt = reinterpret_cast<PageTable*>(page->pfn * PAGE_SIZE);

            // 复制页表内容
            for (int j = 0; j < 1024; j++) {
                dst_pt->entries[j] = src_pt->entries[j];
            }
            
            // 更新目标页目录项，保持相同的访问权限
            dst->entries[i] = reinterpret_cast<uint32_t>(dst_pt) | (src->entries[i] & 0xFFF);
            used_pt++;
        } else {
            // 如果源页目录项不存在，目标也设为不存在
            dst->entries[i] = src->entries[i];
        }
    }
    return used_pt; // 返回使用的页表数量
}

int PageManager::copyMemorySpace(PageDirectory *src, PageDirectory* &out) {
    auto page = Kernel::instance().kernel_mm().alloc_pages(1);
    if (page == nullptr) {
        Console::print("copyKernelSpace: alloc_pages failed");
        return -1;
    }

    PageDirectory* dst = reinterpret_cast<PageDirectory*>(page->virtual_address);
    out = (PageDirectory*)(page->pfn * PAGE_SIZE);
    copyKernelSpace(src, dst);
    // int ret = copyUserSpace(src, dst);
    // if (ret < 0) {
    //     Console::print("copyKernelSpace: copy failed");
    //     Kernel::instance().kernel_mm().free_pages(page, 1);
    //     return -1;
    // }
    delete []page;
    return 0;
}

void PageManager::loadPageDirectory(uint32_t dir) {
    asm volatile("mov %0, %%cr3" : : "r"(dir));
}

void PageManager::enablePaging() {
    uint32_t cr0_val;
    // 获取当前 CR0 寄存器的值
    asm volatile("mov %%cr0, %0" : "=r"(cr0_val));
    cr0_val |= 0x80000000; // 启用分页
    // 将修改后的 CR0 值写回 CR0 寄存器
    asm volatile("mov %0, %%cr0" : : "r"(cr0_val));
}

// 创建页表
PageTable * PageManager::createPageTable() {
    PageTable* table = reinterpret_cast<PageTable*>(nextFreePage);
    for (int i = 0; i < 1024; i++) {
        table->entries[i] = 0x00000002; // Supervisor, read/write, not present
    }
}

// 映射虚拟地址到物理地址
void PageManager::mapPage(uint32_t virt_addr, uint32_t phys_addr, uint32_t flags) {
    uint32_t pd_index = virt_addr >> 22;
    uint32_t pt_index = (virt_addr >> 12) & 0x3FF;

    // 获取页目录项
    if (!currentPageDirectory) return;

    // 检查页表是否存在
    PageTable* pt;
    if (!(currentPageDirectory->entries[pd_index] & 0x1)) {
        // 创建新页表
        pt = createPageTable();
        currentPageDirectory->entries[pd_index] = reinterpret_cast<uint32_t>(pt) | 3; // Supervisor, read/write, present
    } else {
        pt = reinterpret_cast<PageTable*>(currentPageDirectory->entries[pd_index] & 0xFFFFF000);
    }

    // 设置页表项
    pt->entries[pt_index] = (phys_addr & 0xFFFFF000) | (flags & 0xFFF) | 0x1; // Present
}

// 解除虚拟地址映射
void PageManager::unmapPage(uint32_t virt_addr) {
    uint32_t pd_index = virt_addr >> 22;
    uint32_t pt_index = (virt_addr >> 12) & 0x3FF;

    if (!currentPageDirectory || !(currentPageDirectory->entries[pd_index] & 0x1)) return;

    PageTable* pt = reinterpret_cast<PageTable*>(currentPageDirectory->entries[pd_index] & 0xFFFFF000);
    pt->entries[pt_index] = 0x00000002; // Supervisor, read/write, not present
}

// 切换页目录
void PageManager::switchPageDirectory(PageDirectory* dirVirt, void * dirPhys) {
    currentPageDirectory = dirVirt;
    loadPageDirectory(reinterpret_cast<uint32_t>(dirPhys));
}