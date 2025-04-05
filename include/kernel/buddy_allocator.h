#pragma once
#include <cstdint>

class BuddyAllocator
{
public:
    // 初始化伙伴系统分配器
    void init(uint32_t start_addr, uint32_t size);

    uint32_t allocate_pages(uint32_t gfp_mask, uint32_t order);
    void free_pages(uint32_t phys, uint32_t order);
    void increment_ref_count(uint32_t phys, uint32_t order = 0);
    void decrement_ref_count(uint32_t phys, uint32_t order = 0);

private:
    static constexpr uint32_t MIN_ORDER = 0;  // 最小分配单位为1页(4KB)
    static constexpr uint32_t MAX_ORDER = 20; // 最大分配单位为 4GB

    // 空闲内存块链表节点
    struct FreeBlock {
        FreeBlock* next;
        uint32_t size; // 块大小(以页为单位)
    };

    // 每个order对应的空闲链表
    FreeBlock* free_lists[MAX_ORDER + 1];
    uint32_t real_start;
    uint32_t memory_start;
    uint32_t memory_size;

    struct PageInfo {
        uint32_t ref_count;
        bool is_cow;
        bool is_compound;     // 是否为复合页的一部分
        uint32_t compound_order;  // 如果是复合页，记录复合页的order
        uint32_t compound_head;   // 复合页的首页物理地址
    };
    PageInfo* page_info = nullptr;
    uint32_t page_count = 0;

    // 内部辅助函数
    uint32_t get_buddy_address(uint32_t addr, uint32_t size);
    uint32_t get_block_order(uint32_t num_pages);
    void split_block(uint32_t addr, uint32_t order);
    bool try_merge_buddy(uint32_t addr, uint32_t order);
};