#pragma once
#include "arch/x86/paging.h"
#include "kernel/virtual_memory_tree.h"
#include "kernel/zone.h"
#include <stdint.h>



using namespace MemoryConstants;

// 内核内存管理类
class KernelMemory
{
public:
    // 构造函数
    KernelMemory();
    PageManager& paging()
    {
        return page_manager;
    }

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
    uint32_t virt2Phys(void* virt_addr);
    void* phys2Virt(uint32_t phys_addr);

    // 获取虚拟地址对应的页框号
    uint32_t getPfn(void* virt_addr);

    // 分配连续的物理页面
    struct page* alloc_pages(uint32_t count);

    // 使用物理地址
    uint32_t allocPage();
    void freePage(uint32_t physAddr);
    void decrement_ref_count(uint32_t physAddr);
    void increment_ref_count(uint32_t physAddr);

    // 释放已分配的页面
    void free_pages(struct page* pages, uint32_t count);

private:
    // 根据大小选择合适的内存区域
    Zone* get_zone_for_allocation(uint32_t size);

    // 内存区域
    Zone dma_zone;                  // DMA区域
    Zone normal_zone;               // 普通区域
    Zone high_zone;                 // 高端区域
    PageManager page_manager;       // 页表管理器
    VirtualMemoryTree vmalloc_tree; // 虚拟内存树
};