#pragma once
#include <stdint.h>

class BuddyAllocator {
public:
    // 初始化伙伴系统分配器
    static void init(uint32_t start_addr, uint32_t size);

    // 分配指定页数的内存
    static uint32_t allocate_pages(uint32_t num_pages);

    // 释放之前分配的内存
    static void free_pages(uint32_t addr, uint32_t num_pages);

private:
    static constexpr uint32_t MIN_ORDER = 0;  // 最小分配单位为1页(4KB)
    static constexpr uint32_t MAX_ORDER = 14; // 最大分配单位为1024*16页(64MB)
    static constexpr uint32_t PAGE_SIZE = 4096;

    // 空闲内存块链表节点
    struct FreeBlock {
        FreeBlock* next;
        uint32_t size;  // 块大小(以页为单位)
    };

    // 每个order对应的空闲链表
    static FreeBlock* free_lists[MAX_ORDER + 1];
    static uint32_t memory_start;
    static uint32_t memory_size;

    // 内部辅助函数
    static uint32_t get_buddy_address(uint32_t addr, uint32_t size);
    static uint32_t get_block_order(uint32_t num_pages);
    static void split_block(uint32_t addr, uint32_t order);
    static bool try_merge_buddy(uint32_t addr, uint32_t order);
};