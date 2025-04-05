#pragma once
#include "arch/x86/paging.h"
#include "kernel/virtual_memory_tree.h"
#include "kernel/zone.h"
#include "slab_allocator.h"
#include <cstdint>

using namespace MemoryConstants;

// 内核内存管理类
class KernelMemory
{
public:
    // 构造函数
    KernelMemory();
    PageManager& paging() { return page_manager; }

    // 初始化内核内存管理
    void init();

    // 分配虚拟内存
    VADDR kmalloc(uint32_t size);
    void kfree(VADDR addr);
    VADDR vmalloc(uint32_t size);
    void vfree(VADDR addr);
    VADDR kmap(PADDR phys_addr);
    void kunmap(VADDR addr);

    // 分配物理页面
    PADDR alloc_pages(uint32_t gfp_mask, uint32_t order);
    void free_pages(PADDR phys_addr, uint32_t order);
    void decrement_ref_count(PADDR physAddr);
    void increment_ref_count(PADDR physAddr);

    // 地址转换
    PADDR virt2Phys(VADDR virt_addr);
    VADDR phys2Virt(PADDR phys_addr);
    PFN getPfn(VADDR virt_addr);

private:
    // 根据大小选择合适的内存区域
    Zone* get_zone_for_allocation(uint32_t size);

    // 内存区域
    Zone dma_zone;                  // DMA区域
    Zone normal_zone;               // 普通区域
    Zone high_zone;                 // 高端区域
    PageManager page_manager;       // 页表管理器
    kernel::SlabAllocator slab_allocator;
    VirtualMemoryTree vmalloc_tree; // 虚拟内存树
};