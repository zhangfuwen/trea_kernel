#pragma once
#include <stdint.h>
#include "kernel/zone.h"
#include "arch/x86/paging.h"
#include "kernel/virtual_memory_tree.h"
#include "kernel/page.h"

// 4M 以后开始分配内存
// 4M -> 4M + 4K 是PDT
// 4M + 4K -> 4M + 516K 是内存页表，为512K/4个条目，即128K*4K = 1024M = 1GB空间
// 以上是指物理内存，不需要虚拟地址

// 内存区域常量定义
namespace MemoryConstants {
    // 页大小
    constexpr uint32_t PAGE_SIZE = 4096;
    
    // 物理内存区域大小（以页为单位）
    constexpr uint32_t DMA_ZONE_START = 0x0;          // 0MB
    constexpr uint32_t DMA_ZONE_END = 0x1000;        // 16MB (4096页)
    constexpr uint32_t NORMAL_ZONE_START = DMA_ZONE_END;
    constexpr uint32_t NORMAL_ZONE_END = 0x38000;    // 896MB (229376页)
    constexpr uint32_t HIGH_ZONE_START = NORMAL_ZONE_END;
    constexpr uint32_t HIGH_ZONE_END = 0x100000;     // 4GB (1048576页)
    
    // 虚拟地址空间布局
    constexpr uint32_t KERNEL_DIRECT_MAP_START = 0xC0000000;  // 3GB (内核空间起始地址)
    constexpr uint32_t KERNEL_DIRECT_MAP_END = 0xF8000000;    // 3GB + 896MB (直接映射区，用于映射DMA_ZONE和NORMAL_ZONE)
    constexpr uint32_t VMALLOC_START = KERNEL_DIRECT_MAP_END; // 3GB + 896MB
    constexpr uint32_t VMALLOC_END = 0xF8000000;             // 3GB + 896MB (VMALLOC区域，64MB，用于非连续内存分配)
    constexpr uint32_t KMAP_START = 0xF8000000;              // 3GB + 896MB
    constexpr uint32_t KMAP_END = 0xFC000000;                // 3GB + 960MB (KMAP区域，64MB，用于临时内核映射)
}

using namespace MemoryConstants;

// 内核内存管理类
class KernelMemory {
public:
    // 构造函数
    KernelMemory();
    PageManager &paging() { return page_manager; }

    // 初始化内核内存管理
    void init();

    // 分配小块连续物理内存（返回虚拟地址）
    void* kmalloc(uint32_t size);

    // 释放通过kmalloc分配的内存
    void kfree(void* addr);

    // 分配大块非连续虚拟内存
    void* vmalloc(uint32_t size);

    // 释放通过vmalloc分配的内存
    void vfree(void* addr);

    // 将物理页面临时映射到内核空间
    void* kmap(uint32_t phys_addr);

    // 解除kmap的映射
    void kunmap(void* addr);

    // 获取虚拟地址对应的物理地址
    uint32_t getPhysicalAddress(void* virt_addr);
    void * getVirtualAddress(uint32_t phys_addr);

    // 获取虚拟地址对应的页框号
    uint32_t getPfn(void* virt_addr);

    // 分配连续的物理页面
    struct page* alloc_pages(uint32_t count);

    // 释放已分配的页面
    void free_pages(struct page* pages, uint32_t count);

private:
    // 根据大小选择合适的内存区域
    Zone* get_zone_for_allocation(uint32_t size);

    // 内存区域
    Zone dma_zone;      // DMA区域
    Zone normal_zone;   // 普通区域
    Zone high_zone;     // 高端区域
    PageManager page_manager; // 页表管理器
    VirtualMemoryTree vmalloc_tree; // 虚拟内存树

};