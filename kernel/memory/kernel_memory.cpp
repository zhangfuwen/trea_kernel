#include <kernel/kernel_memory.h>
#include <lib/serial.h>

#include "arch/x86/paging.h"
#include "lib/debug.h"

KernelMemory::KernelMemory()
    : dma_zone(), normal_zone(), high_zone(), page_manager(),
      vmalloc_tree(VMALLOC_START, VMALLOC_END)
{
    debug_debug("KernelMemory::KernelMemory()");
}

// 分配连续的物理页面
PADDR KernelMemory::alloc_pages(uint32_t gfp_mask, uint32_t order)
{
    if(order > 20) { // MAX_ORDER is 20
        debug_debug("order %d too large\n", order);
        return 0;
    }

    // 根据页面数量选择合适的区域
    uint32_t size = (1 << order) * PAGE_SIZE;
    Zone* zone = get_zone_for_allocation(size);
    if(!zone) {
        debug_debug("get_zone_for_allocation failed for size %d\n", size);
        return 0;
    }

    // 从区域中分配物理页面
    uint32_t pfn = zone->allocPages(gfp_mask, order);
    if(!pfn) {
        debug_debug("allocPages failed\n");
        return 0;
    }

    PADDR phys_addr = pfn * PAGE_SIZE;
    debug_debug("KernelMemory::alloc_pages() addr: 0x%x\n", phys_addr);
    return phys_addr;
}

// 释放已分配的页面
void KernelMemory::free_pages(PADDR phys_addr, uint32_t order)
{
    if(phys_addr == 0 || order > 20) { // MAX_ORDER is 20
        return;
    }

    uint32_t pfn = phys_addr / PAGE_SIZE;
    Zone* zone = nullptr;

    // 根据PFN确定页面所属的区域
    if(pfn < DMA_ZONE_END) {
        zone = &dma_zone;
    } else if(pfn < NORMAL_ZONE_END) {
        zone = &normal_zone;
    } else {
        zone = &high_zone;
    }

    // 释放物理页面
    zone->freePages(pfn, order);
}

// 释放已分配的页面
void KernelMemory::decrement_ref_count(PADDR physAddr)
{
    uint32_t pfn = physAddr / PAGE_SIZE;
    Zone* zone = nullptr;

    // 根据PFN确定页面所属的区域
    if(pfn < DMA_ZONE_END) {
        zone = &dma_zone;
    } else if(pfn < NORMAL_ZONE_END) {
        zone = &normal_zone;
    } else {
        zone = &high_zone;
    }

    // 释放物理页面
    zone->decRefPage(pfn);
}
void KernelMemory::increment_ref_count(PADDR physAddr)
{
    uint32_t pfn = physAddr / PAGE_SIZE;
    Zone* zone = nullptr;

    // 根据PFN确定页面所属的区域
    if(pfn < DMA_ZONE_END) {
        zone = &dma_zone;
    } else if(pfn < NORMAL_ZONE_END) {
        zone = &normal_zone;
    } else {
        zone = &high_zone;
    }

    // 释放物理页面
    zone->increment_ref_count(pfn);
}

// 初始化内核内存管理
void KernelMemory::init()
{
    serial_puts("KernelMemory::init()\n");
    page_manager.init();
    // // 建立直接映射区的页表映射（包括DMA区域和普通区域）
    // for (uint32_t phys_addr = 0; phys_addr < NORMAL_ZONE_END * PAGE_SIZE;
    // phys_addr += PAGE_SIZE) {
    //     uint32_t virt_addr = KERNEL_DIRECT_MAP_START + phys_addr;
    //     page_manager.mapPage(virt_addr, phys_addr, 3); // Supervisor,
    //     read/write, present
    // }

    serial_puts("KernelMemory::init() 2\n");
    // 初始化各个内存区域
    // dma_zone.init(ZoneType::ZONE_DMA, DMA_ZONE_START, DMA_ZONE_END);
    normal_zone.init(ZoneType::ZONE_NORMAL, NORMAL_ZONE_START, NORMAL_ZONE_END);
    // high_zone.init(ZoneType::ZONE_HIGH, HIGH_ZONE_START, HIGH_ZONE_END);
    serial_puts("KernelMemory::init() 3");

    slab_allocator.init();

    // 初始化VMALLOC区域
    //    vmalloc_tree.init();
}

// 分配小块连续物理内存（返回虚拟地址）
VADDR KernelMemory::kmalloc(uint32_t size)
{
    return slab_allocator.kmalloc(size);
    // // 根据大小选择合适的区域
    // Zone* zone = get_zone_for_allocation(size);
    // if(!zone) {
    //     debug_debug("ProcessManager: Failed to allocate kernel memory, size: %d\n", size);
    //     return nullptr;
    // }
    //
    // // 计算需要的页数
    // uint32_t pages = (size + PAGE_SIZE - 1) >> 12;
    //
    // // 分配物理页面
    // uint32_t pfn = zone->allocPages(pages);
    // if(!pfn) {
    //     debug_debug("ProcessManager: Failed to allocate kernel memory\n");
    //     return nullptr;
    // }
    //
    // // 计算物理地址和虚拟地址
    // uint32_t phys_addr = pfn << 12;
    // uint32_t virt_addr = KERNEL_DIRECT_MAP_START + phys_addr; // 内核空间映射
    //
    // // 只有高端内存区域需要建立页表映射
    // if(zone == &high_zone) {
    //     for(uint32_t i = 0; i < pages; i++) {
    //         page_manager.mapPage(virt_addr + i * PAGE_SIZE, phys_addr + i * PAGE_SIZE,
    //             3); // Supervisor, read/write, present
    //     }
    // }
    //
    // return (void*)virt_addr;
}

// 释放通过kmalloc分配的内存
void KernelMemory::kfree(VADDR addr)
{
    slab_allocator.kfree(addr);
    // if(!addr)
    //     return;
    //
    // // 获取物理页框号
    // uint32_t pfn = (uint32_t)addr >> 12;
    //
    // // 找到对应的区域并释放
    // if(pfn < dma_zone.getWatermark(WatermarkLevel::WMARK_HIGH)) {
    //     dma_zone.freePages(pfn, 1);
    // } else if(pfn < normal_zone.getWatermark(WatermarkLevel::WMARK_HIGH)) {
    //     normal_zone.freePages(pfn, 1);
    // } else {
    //     high_zone.freePages(pfn, 1);
    // }
}

// 分配大块非连续虚拟内存
VADDR KernelMemory::vmalloc(uint32_t size)
{
    if(size == 0)
        return nullptr;

    // 计算需要的页数
    uint32_t pages = (size + PAGE_SIZE - 1) >> 12;

    // 从虚拟内存树中分配虚拟地址空间
    uint32_t virt_addr = vmalloc_tree.allocate(pages * PAGE_SIZE);
    if(!virt_addr) {
        return nullptr;
    }

    // 逐页分配物理内存并建立映射
    for(uint32_t i = 0; i < pages; i++) {
        // 优先从普通区域分配物理页面
        Zone* zone = &normal_zone;
        uint32_t pfn = zone->allocPages(0, 0);

        // 如果普通区域分配失败，尝试从高端区域分配
        if(!pfn) {
            zone = &high_zone;
            pfn = zone->allocPages(0, 0);

            // 如果高端区域也分配失败，回滚并返回
            if(!pfn) {
                // 释放已分配的页面
                for(uint32_t j = 0; j < i; j++) {
                    uint32_t prev_virt = virt_addr + j * PAGE_SIZE;
                    uint32_t prev_phys = virt2Phys((void*)prev_virt);
                    page_manager.unmapPage(prev_virt);
                    if(prev_phys < normal_zone.getWatermark(WatermarkLevel::WMARK_HIGH)) {
                        normal_zone.freePages(prev_phys >> 12, 1);
                    } else {
                        high_zone.freePages(prev_phys >> 12, 1);
                    }
                }
                vmalloc_tree.free(virt_addr);
                return nullptr;
            }
        }

        // 建立映射
        uint32_t phys_addr = pfn << 12;
        page_manager.mapPage(virt_addr + i * PAGE_SIZE, phys_addr, 3);
    }

    return (void*)virt_addr;
}

// 释放通过vmalloc分配的内存
void KernelMemory::vfree(VADDR addr)
{
    if(!addr)
        return;

    uint32_t virt_addr = (uint32_t)addr;

    // 确保地址在VMALLOC区域
    if(virt_addr < VMALLOC_START || virt_addr >= VMALLOC_END) {
        return;
    }

    // 逐页处理，直到遇到未映射的页面
    uint32_t current_addr = virt_addr;
    while(true) {
        // 获取物理地址
        uint32_t phys_addr = virt2Phys((void*)current_addr);
        if(!phys_addr)
            break; // 遇到未映射的页面

        // 获取物理页框号
        uint32_t pfn = phys_addr >> 12;

        // 解除页表映射
        page_manager.unmapPage(current_addr);

        // 找到对应的区域并释放物理页面
        Zone* zone = get_zone_for_allocation(PAGE_SIZE);
        if(zone) {
            zone->freePages(pfn, 0); // 释放单个页面，order为0
        }

        current_addr += PAGE_SIZE;
    }

    // 释放虚拟地址空间
    vmalloc_tree.free(virt_addr);
}

// 将物理页面临时映射到内核空间
VADDR KernelMemory::kmap(PADDR phys_addr)
{
    // 从KMAP区域分配虚拟地址
    static uint32_t next_kmap_addr = KMAP_START;
    uint32_t virt_addr = next_kmap_addr;
    next_kmap_addr += PAGE_SIZE;

    // 建立临时映射
    page_manager.mapPage(virt_addr, phys_addr & ~(PAGE_SIZE - 1), 3);

    return (void*)(virt_addr | (phys_addr & 0xFFF));
}

// 解除kmap的映射
void KernelMemory::kunmap(VADDR addr)
{
    if(!addr)
        return;

    uint32_t virt_addr = (uint32_t)addr & ~0xFFF;
    if(virt_addr >= KMAP_START && virt_addr < KMAP_END) {
        page_manager.unmapPage(virt_addr);
    }
}

// 获取虚拟地址对应的物理地址
PADDR KernelMemory::virt2Phys(VADDR virt_addr)
{
    return (uint32_t)virt_addr - KERNEL_DIRECT_MAP_START;
}
VADDR KernelMemory::phys2Virt(PADDR phys_addr)
{
    return (void*)(phys_addr + KERNEL_DIRECT_MAP_START);
}

// 获取虚拟地址对应的页框号
PFN KernelMemory::getPfn(VADDR virt_addr)
{
    uint32_t phys_addr = virt2Phys(virt_addr);
    return phys_addr >> 12; // 右移12位得到页框号
}

// 根据大小选择合适的内存区域
Zone* KernelMemory::get_zone_for_allocation(uint32_t size)
{
    // 对于小于16MB的分配，优先使用DMA区域
    //    if (size <= DMA_ZONE_END * PAGE_SIZE) {
    //        if (dma_zone.getFreePages() * PAGE_SIZE >= size) {
    //            return &dma_zone;
    //        }
    //    }

    // 对于普通大小的分配，使用普通区域
    if(normal_zone.getFreePages() * PAGE_SIZE >= size) {
        return &normal_zone;
    } else {
        debug_debug("num free pages: %d\n", normal_zone.getFreePages());
    }

    //    // 对于大块内存，使用高端区域
    //    if (high_zone.getFreePages() * PAGE_SIZE >= size) {
    //        return &high_zone;
    //    }

    return nullptr;
}