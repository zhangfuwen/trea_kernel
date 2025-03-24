#include "kernel/buddy_allocator.h"
#include "lib/debug.h"

void BuddyAllocator::init(uint32_t start_addr, uint32_t size) {
    memory_start = start_addr;
    memory_size = size;

    // 初始化所有空闲链表为空
    for (int i = 0; i <= MAX_ORDER; i++) {
        free_lists[i] = nullptr;
    }

    // 将整个内存区域作为一个大块加入到合适的空闲链表中
    uint32_t total_pages = size / PAGE_SIZE;
    uint32_t order = get_block_order(total_pages);
    FreeBlock* block = (FreeBlock*)start_addr;
    block->next = nullptr;
    block->size = 1 << order;
    debug_debug("BuddyAllocator: init, block size: %d, order:%d\n", block->size, order);
    free_lists[order] = block;
}

uint32_t BuddyAllocator::allocate_pages(uint32_t num_pages) {
    // 计算需要的块大小的order
    uint32_t order = get_block_order(num_pages);
//    debug_debug("order: %d\n", order);
    if (order > MAX_ORDER) {
        debug_debug("BuddyAllocator: Requested size is too large!\n");    
        return 0;
    }

    // 查找可用的最小块
    uint32_t current_order = order;
    while (current_order <= MAX_ORDER && !free_lists[current_order]) {
        current_order++;
    }
//    debug_debug("current order: %d\n", current_order);

    // 如果没有找到足够大的块
    if (current_order > MAX_ORDER) {
        debug_debug("BuddyAllocator: No available blocks!, current order: %d, order:%d\n", current_order, order);
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
    while (current_order > order) {
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

//    debug_debug("block3: %x\n", block);
    return (uint32_t)block;
}

void BuddyAllocator::free_pages(uint32_t addr, uint32_t num_pages) {
    uint32_t order = get_block_order(num_pages);
    
    // 尝试合并伙伴块
    while (order < MAX_ORDER) {
        uint32_t buddy_addr = get_buddy_address(addr, 1 << order);
        bool merged = try_merge_buddy(addr, order);
        if (!merged) break;
        
        // 如果合并成功，移除伙伴块并继续尝试合并更大的块
        addr = addr < buddy_addr ? addr : buddy_addr;
        order++;
    }

    // 将最终的块添加到对应的空闲链表中
    FreeBlock* block = (FreeBlock*)addr;
    block->size = 1 << order;
    block->next = free_lists[order];
    free_lists[order] = block;
}

uint32_t BuddyAllocator::get_buddy_address(uint32_t addr, uint32_t size) {
    return addr ^ (size * PAGE_SIZE);
}

uint32_t BuddyAllocator::get_block_order(uint32_t num_pages) {
    uint32_t order = 0;
    num_pages--;
    while (num_pages > 0) {
        num_pages >>= 1;
        order++;
    }
    return order;
}

void BuddyAllocator::split_block(uint32_t addr, uint32_t order) {
    if (order == 0) return;

    uint32_t buddy_addr = addr + (1 << (order - 1)) * PAGE_SIZE;
    FreeBlock* buddy = (FreeBlock*)buddy_addr;
    buddy->size = 1 << (order - 1);
    buddy->next = free_lists[order - 1];
    free_lists[order - 1] = buddy;
}

bool BuddyAllocator::try_merge_buddy(uint32_t addr, uint32_t order) {
    uint32_t buddy_addr = get_buddy_address(addr, 1 << order);
    
    // 检查伙伴块是否在空闲链表中
    FreeBlock** current = &free_lists[order];
    while (*current) {
        if ((uint32_t)*current == buddy_addr) {
            // 找到伙伴块，从链表中移除
            *current = (*current)->next;
            return true;
        }
        current = &(*current)->next;
    }
    
    return false;
}