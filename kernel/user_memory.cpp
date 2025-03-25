#include <kernel/user_memory.h>
#include <arch/x86/paging.h>
#include <lib/string.h>

// 物理页面分配和释放函数声明
uint32_t allocate_physical_page();
void free_physical_page(uint32_t page);

// 初始化内存管理器
void UserMemory::init(uint32_t page_dir) {
    pgd = page_dir;
    num_areas = 0;
    total_vm = 0;
    locked_vm = 0;
    
    // 初始化内存区域数组
    memset(areas, 0, sizeof(areas));
}

// 分配一个新的内存区域
// 使用first-fit策略查找合适的空闲区域
uint32_t UserMemory::find_free_area(uint32_t size) {
    // 确保大小按页对齐
    size = (size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    
    // 如果没有已分配区域，从用户空间起始地址开始分配
    if (num_areas == 0) {
        return USER_START;
    }
    
    // 检查用户空间起始位置到第一个区域之间的空间
    if (areas[0].start_addr > USER_START && 
        areas[0].start_addr - USER_START >= size) {
        return USER_START;
    }
    
    // 检查相邻区域之间的空隙
    for (uint32_t i = 0; i < num_areas - 1; i++) {
        uint32_t gap_start = areas[i].end_addr;
        uint32_t gap_size = areas[i + 1].start_addr - gap_start;
        
        if (gap_size >= size) {
            return gap_start;
        }
    }
    
    // 检查最后一个区域之后到用户空间结束位置之间的空间
    uint32_t last_end = areas[num_areas - 1].end_addr;
    if (last_end < USER_END && USER_END - last_end >= size) {
        return last_end;
    }
    
    return 0; // 没有找到合适的空闲区域
}

bool UserMemory::allocate_area(uint32_t size, uint32_t flags, uint32_t type) {
    if (num_areas >= MAX_MEMORY_AREAS) {
        return false;
    }
    
    // 查找合适的空闲区域
    uint32_t start = find_free_area(size);
    if (start == 0) {
        return false;
    }
    
    // 确保大小按页对齐
    size = (size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    uint32_t end = start + size;
    
    // 添加新的内存区域
    areas[num_areas].start_addr = start;
    areas[num_areas].end_addr = end;
    areas[num_areas].flags = flags;
    areas[num_areas].type = type;
    num_areas++;
    
    // 更新总虚拟内存大小
    total_vm += size >> 12; // 已经按页对齐，直接除以页大小
    
    return true;
}

// 释放指定地址范围的内存区域
void UserMemory::free_area(uint32_t start, uint32_t size) {
    uint32_t end = start + size;
    
    // 查找并移除匹配的内存区域
    for (uint32_t i = 0; i < num_areas; i++) {
        if (areas[i].start_addr == start && areas[i].end_addr == end) {
            // 更新总虚拟内存大小
            total_vm -= (size + 0xFFF) >> 12;
            
            // 移动后续区域
            for (uint32_t j = i; j < num_areas - 1; j++) {
                areas[j] = areas[j + 1];
            }
            num_areas--;
            break;
        }
    }
    
    // 解除该区域的页面映射
    unmap_pages(start, size);
}

// 扩展或收缩堆区
uint32_t UserMemory::brk(uint32_t new_brk) {
    if (new_brk < start_heap) {
        return end_heap; // 不允许小于堆的起始地址
    }
    
    if (new_brk > start_stack) {
        return end_heap; // 不允许超过栈区
    }
    
    // 计算需要增加或减少的页数
    int32_t old_pages = (end_heap - start_heap + 0xFFF) >> 12;
    int32_t new_pages = (new_brk - start_heap + 0xFFF) >> 12;
    
    if (new_pages > old_pages) {
        // 需要扩展堆
        for (uint32_t addr = end_heap; addr < new_brk; addr += 0x1000) {
            // 分配物理页面并建立映射
            uint32_t phys_page = allocate_physical_page();
            if (!map_pages(addr, phys_page, 0x1000, PAGE_USER | PAGE_WRITE)) {
                return end_heap;
            }
        }
    } else if (new_pages < old_pages) {
        // 需要收缩堆
        unmap_pages(new_brk, end_heap - new_brk);
    }
    
    end_heap = new_brk;
    return end_heap;
}

// 映射物理页面到虚拟地址空间
bool UserMemory::map_pages(uint32_t virt_addr, uint32_t phys_addr, uint32_t size, uint32_t flags) {
    uint32_t num_pages = (size + 0xFFF) >> 12;
    
    for (uint32_t i = 0; i < num_pages; i++) {
        uint32_t vaddr = virt_addr + (i << 12);
        uint32_t paddr = phys_addr + (i << 12);
        
        // 获取页目录项和页表项的索引
        uint32_t pde_idx = vaddr >> 22;
        uint32_t pte_idx = (vaddr >> 12) & 0x3FF;
        
        // 获取页目录项
        uint32_t* pde = (uint32_t*)(pgd + (pde_idx << 2));
        
        // 如果页表不存在，创建新的页表
        if (!(*pde & PAGE_PRESENT)) {
            uint32_t page_table = allocate_physical_page();
            *pde = page_table | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
            memset((void*)page_table, 0, 0x1000);
        }
        
        // 获取页表
        uint32_t page_table = *pde & 0xFFFFF000;
        uint32_t* pte = (uint32_t*)(page_table + (pte_idx << 2));
        
        // 建立页表项映射
        *pte = paddr | flags | PAGE_PRESENT;
    }
    
    return true;
}

// 解除虚拟地址空间的映射
void UserMemory::unmap_pages(uint32_t virt_addr, uint32_t size) {
    uint32_t num_pages = (size + 0xFFF) >> 12;
    
    for (uint32_t i = 0; i < num_pages; i++) {
        uint32_t vaddr = virt_addr + (i << 12);
        
        // 获取页目录项和页表项的索引
        uint32_t pde_idx = vaddr >> 22;
        uint32_t pte_idx = (vaddr >> 12) & 0x3FF;
        
        // 获取页目录项
        uint32_t* pde = (uint32_t*)(pgd + (pde_idx << 2));
        
        if (*pde & PAGE_PRESENT) {
            // 获取页表
            uint32_t page_table = *pde & 0xFFFFF000;
            uint32_t* pte = (uint32_t*)(page_table + (pte_idx << 2));
            
            // 清除页表项
            if (*pte & PAGE_PRESENT) {
                uint32_t phys_page = *pte & 0xFFFFF000;
                free_physical_page(phys_page);
                *pte = 0;
            }
        }
    }
}

// 查找最大的连续空闲区域
uint32_t UserMemory::find_largest_free_area() {
    uint32_t largest_size = 0;
    
    // 如果没有已分配区域，返回用户空间的总大小
    if (num_areas == 0) {
        return USER_END - USER_START;
    }
    
    // 检查用户空间起始位置到第一个区域之间的空间
    if (areas[0].start_addr > USER_START) {
        largest_size = areas[0].start_addr - USER_START;
    }
    
    // 检查相邻区域之间的空隙
    for (uint32_t i = 0; i < num_areas - 1; i++) {
        uint32_t gap_start = areas[i].end_addr;
        uint32_t gap_size = areas[i + 1].start_addr - gap_start;
        
        if (gap_size > largest_size) {
            largest_size = gap_size;
        }
    }
    
    // 检查最后一个区域之后到用户空间结束位置之间的空间
    uint32_t last_end = areas[num_areas - 1].end_addr;
    if (last_end < USER_END) {
        uint32_t final_gap = USER_END - last_end;
        if (final_gap > largest_size) {
            largest_size = final_gap;
        }
    }
    
    return largest_size;
}