#pragma once
#include <stdint.h>

// 页面状态标志
enum class PageFlags : uint32_t {
    PAGE_RESERVED = 1 << 0,    // 保留页面
    PAGE_ALLOCATED = 1 << 1,   // 已分配
    PAGE_DIRTY = 1 << 2,       // 脏页面
    PAGE_LOCKED = 1 << 3,      // 锁定页面
    PAGE_REFERENCED = 1 << 4,  // 被引用
    PAGE_ACTIVE = 1 << 5,      // 活跃页面
    PAGE_INACTIVE = 1 << 6,    // 不活跃页面
};

// 物理页面描述符
struct page {
    uint32_t flags;            // 页面状态标志
    uint32_t _count;           // 引用计数
    uint32_t virtual_address;  // 映射的虚拟地址
    uint32_t pfn;             // 页框号
    struct page* next;         // 链表指针，用于空闲页面链表
};