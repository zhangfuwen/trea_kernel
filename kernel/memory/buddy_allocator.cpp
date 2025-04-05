#include "kernel/buddy_allocator.h"

#include <kernel/kernel.h>

#include "lib/debug.h"

void BuddyAllocator::init(uint32_t start_addr, uint32_t size)
{
    debug_info("BuddyAllocator::init(start_addr 0x%x, size:%d(0x%x))\n", start_addr, size, size);
    // 先分配PageInfo元数据空间
    page_count = size / PAGE_SIZE;
    uint32_t info_bytes = page_count * sizeof(PageInfo);
    info_bytes = (info_bytes + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1); // 按页对齐

    // 调整实际管理的内存区域
    real_start = start_addr;
    memory_start = start_addr + info_bytes;
    memory_size = size - info_bytes;
    debug_info("page_info size:%d(0x%x), memory_start:0x%x\n", info_bytes, info_bytes, memory_start);

    // 设置page_info指针并初始化
    page_info = reinterpret_cast<PageInfo*>(Kernel::instance().kernel_mm().phys2Virt(start_addr));
    debug_debug("memset page_info(0x%x, phys:0x%x), size:%d(0x%x)\n", page_info, start_addr, info_bytes, info_bytes);
    memset(page_info, 0, info_bytes);
    debug_debug("memset page_info done\n");


    // 初始化所有空闲链表为空
    debug_debug("init free_lists\n");
    for(int i = 0; i <= MAX_ORDER; i++) {
        free_lists[i] = nullptr;
    }

    // 将整个内存区域作为一个大块加入到合适的空闲链表中
    // 修改这里使用的总页数为调整后的值
    uint32_t total_pages = memory_size / PAGE_SIZE;
    uint32_t order = get_block_order(total_pages);

    debug_debug("total_pages:%d\n", total_pages);
    FreeBlock* block = (FreeBlock*)Kernel::instance().kernel_mm().phys2Virt(memory_start);
    block->next = nullptr;
    block->size = 1 << order;
    debug_debug("BuddyAllocator: init, block size: %d, order:%d\n", block->size, order);
    free_lists[order] = block;
}

uint32_t BuddyAllocator::allocate_pages(uint32_t gfp_mask, uint32_t order)
{
    // 检查order是否超出范围
    if(order > MAX_ORDER) {
        debug_debug("BuddyAllocator: Requested order %d is too large!\n", order);
        return 0;
    }

    // 计算页面数量
    uint32_t num_pages = 1 << order;

    // 查找可用的最小块
    uint32_t current_order = order;
    while(current_order <= MAX_ORDER && !free_lists[current_order]) {
        current_order++;
    }
    //    debug_debug("current order: %d\n", current_order);

    // 如果没有找到足够大的块
    if(current_order > MAX_ORDER) {
        debug_debug("BuddyAllocator: No available blocks!, current order: %d, order:%d\n",
            current_order, order);
        return 0;
    }

    // 获取块并从空闲链表中移除
    FreeBlock* block = free_lists[current_order];
    //    debug_debug("block: %x\n", block);
    if(block == nullptr) {
        debug_debug("BuddyAllocator:  invalid block, current_order: %d!\n", current_order);
        return 0;
    }
    //    debug_debug("block1: %x\n", block);
    free_lists[current_order] = block->next;
    //    debug_debug("block2: %x\n", block);

    // 如果块太大，需要分割
    while(current_order > order) {
        current_order--;
        //     debug_debug("start current order:%d\n", current_order);
        uint32_t buddy_addr = (uint32_t)block + (1 << current_order) * PAGE_SIZE;
        block->size = 1 << current_order;
        FreeBlock* buddy = (FreeBlock*)buddy_addr;
        //      debug_debug("buddy addr: %x\n", buddy_addr);
        buddy->size = 1 << current_order;
        buddy->next = free_lists[current_order];
        free_lists[current_order] = buddy;
        //       debug_debug("end");
    }

    // debug_debug("block3: %x, count:%d\n", block, num_pages);
    auto block_phys = (uint32_t)Kernel::instance().kernel_mm().virt2Phys(block);
    
    // 设置复合页信息
    uint32_t start_index = (block_phys - memory_start) / PAGE_SIZE;
    for(uint32_t i = 0; i < num_pages; i++) {
        page_info[start_index + i].is_compound = true;
        page_info[start_index + i].compound_order = order;
        page_info[start_index + i].compound_head = block_phys;
    }

    increment_ref_count((uint32_t)block_phys);
    return block_phys;
}

void BuddyAllocator::free_pages(uint32_t phys, uint32_t order)
{
    // 验证地址是否有效
    if(phys < memory_start || phys >= (memory_start + memory_size) ||
        (phys % PAGE_SIZE != 0)) {
        debug_err("Invalid phys address: 0x%x\n", phys);
        return;
    }

    // 检查所有页面的引用计数
    uint32_t num_pages = 1 << order;
    uint32_t start_index = (phys - memory_start) / PAGE_SIZE;
    
    // 清除复合页信息并将引用计数清零
    for(uint32_t i = 0; i < num_pages; i++) {
        page_info[start_index + i].is_compound = false;
        page_info[start_index + i].compound_order = 0;
        page_info[start_index + i].compound_head = 0;
        page_info[start_index + i].ref_count = 0;
    }

    // 转换为虚拟地址并尝试合并
    auto virt = (uint32_t)Kernel::instance().kernel_mm().phys2Virt(phys);
    uint32_t current_order = order;

    // 尝试合并伙伴块
    while(current_order < MAX_ORDER) {
        uint32_t buddy_addr = get_buddy_address(virt, 1 << current_order);
        bool merged = try_merge_buddy(virt, current_order);
        if(!merged)
            break;

        // 如果合并成功，移除伙伴块并继续尝试合并更大的块
        virt = virt < buddy_addr ? virt : buddy_addr;
        current_order++;
    }

    // 将最终的块添加到对应的空闲链表中
    FreeBlock* block = (FreeBlock*)virt;
    block->size = 1 << current_order;
    block->next = free_lists[current_order];
    free_lists[current_order] = block;
}

uint32_t BuddyAllocator::get_buddy_address(uint32_t addr, uint32_t size)
{
    return addr ^ (size * PAGE_SIZE);
}

uint32_t BuddyAllocator::get_block_order(uint32_t num_pages)
{
    uint32_t order = 0;
    num_pages--;
    while(num_pages > 0) {
        num_pages >>= 1;
        order++;
    }
    return order;
}

void BuddyAllocator::split_block(uint32_t addr, uint32_t order)
{
    if(order == 0)
        return;

    uint32_t buddy_addr = addr + (1 << (order - 1)) * PAGE_SIZE;
    FreeBlock* buddy = (FreeBlock*)buddy_addr;
    buddy->size = 1 << (order - 1);
    buddy->next = free_lists[order - 1];
    free_lists[order - 1] = buddy;
}

bool BuddyAllocator::try_merge_buddy(uint32_t addr, uint32_t order)
{
    uint32_t buddy_addr = get_buddy_address(addr, 1 << order);

    // 检查伙伴块是否在空闲链表中
    FreeBlock** current = &free_lists[order];
    while(*current) {
        if((uint32_t)*current == buddy_addr) {
            // 找到伙伴块，从链表中移除
            *current = (*current)->next;
            return true;
        }
        current = &(*current)->next;
    }

    return false;
}

void BuddyAllocator::increment_ref_count(uint32_t phys, uint32_t order)
{
    // 验证地址在管理范围内且是合法页对齐地址
    if(phys < real_start || phys >= (memory_start + memory_size) || (phys % PAGE_SIZE != 0)) {
        debug_err("Invalid phys address: 0x%x, memory start:0x%x, end:0x%x\n", phys, real_start,
            memory_start + memory_size);
        return;
    }
    uint32_t index = phys / PAGE_SIZE;
    
    // 如果是复合页的一部分，增加复合页首页的引用计数
    if(page_info[index].is_compound) {
        uint32_t head_index = page_info[index].compound_head / PAGE_SIZE;
        page_info[head_index].ref_count++;
    } else {
        // 如果指定了order，将其标记为复合页
        if(order > 0) {
            uint32_t num_pages = 1 << order;
            for(uint32_t i = 0; i < num_pages; i++) {
                page_info[index + i].is_compound = true;
                page_info[index + i].compound_order = order;
                page_info[index + i].compound_head = phys;
            }
            page_info[index].ref_count++;
        } else {
            page_info[index].ref_count++;
        }
    }
}


void BuddyAllocator::decrement_ref_count(uint32_t phys, uint32_t order)
{
    if(phys < real_start || phys >= (memory_start + memory_size) || (phys % PAGE_SIZE != 0)) {
        debug_err("Invalid phys address: 0x%x\n", phys);
        return;
    }
    uint32_t index = phys / PAGE_SIZE;
    
    // 如果order > 0，表示对整个复合页进行操作
    if(order > 0) {
        // 确保这是一个复合页的首页
        if(!page_info[index].is_compound || page_info[index].compound_head != phys) {
            debug_err("Invalid compound page head: 0x%x\n", phys);
            return;
        }
        
        // 减少复合页首页的引用计数
        if(--page_info[index].ref_count == 0) {
            // 引用计数为0时释放整个复合页
            free_pages(phys, order);
        }
        return;
    }
    
    // order = 0，表示单页操作
    if(page_info[index].is_compound) {
        // 如果是复合页的一部分，需要将其拆分出来
        uint32_t head_index = page_info[index].compound_head / PAGE_SIZE;
        
        // 清除复合页标记
        page_info[index].is_compound = false;
        page_info[index].compound_order = 0;
        page_info[index].compound_head = 0;
        page_info[index].ref_count = 1; // 设置初始引用计数为1
        
        // 减少引用计数并在必要时释放页面
        if(--page_info[index].ref_count == 0) {
            free_pages(phys, 0);
        }
    } else {
        // 普通页面，直接减少引用计数
        if(--page_info[index].ref_count == 0) {
            free_pages(phys, 0);
        }
    }
}