#pragma once
#include <stdint.h>

// 内存区域描述符
struct MemoryArea {
    uint32_t start_addr;     // 起始地址
    uint32_t end_addr;       // 结束地址
    uint32_t flags;          // 访问权限标志
    uint32_t type;           // 区域类型(代码段、数据段、堆、栈等)
};

// 进程虚拟地址空间管理器
class UserMemory {
public:
    // 初始化内存管理器
    void init(uint32_t page_dir, uint32_t(*alloc_page)(), void(*free_page)(uint32_t), void*(*phys_to_virt)(uint32_t));
    
    // 分配一个新的内存区域
    void * allocate_area(uint32_t size, uint32_t flags, uint32_t type);
    // 释放指定地址范围的内存区域
    void free_area(uint32_t start);


    // 扩展或收缩堆区
    uint32_t brk(uint32_t new_brk);
    
    // 映射物理页面到虚拟地址空间
    bool map_pages(uint32_t virt_addr, uint32_t phys_addr, uint32_t size, uint32_t flags);
    
    // 解除虚拟地址空间的映射
    void unmap_pages(uint32_t virt_addr, uint32_t size);

private:
    // 物理页面分配和释放函数声明
    uint32_t (*allocate_physical_page)() = nullptr;
    void (*free_physical_page)(uint32_t page) = nullptr;
    void* (*phys_to_virt)(uint32_t phys_addr) = nullptr;
    // 使用first-fit策略查找合适的空闲区域
    uint32_t find_free_area(uint32_t size);
    
    // 查找最大的连续空闲区域
    uint32_t find_largest_free_area();
    
    static const uint32_t MAX_MEMORY_AREAS = 32;  // 最大内存区域数
    static const uint32_t PAGE_SIZE = 0x1000;     // 页面大小
    static const uint32_t USER_START = 0x40000000; // 用户空间起始地址
    static const uint32_t USER_END = 0xC0000000;  // 用户空间结束地址

    uint32_t pgd;                    // 页目录基地址
    uint32_t start_code;             // 代码段起始地址
    uint32_t end_code;               // 代码段结束地址
    uint32_t start_data;             // 数据段起始地址
    uint32_t end_data;               // 数据段结束地址
    uint32_t start_heap;             // 堆区起始地址
    uint32_t end_heap;               // 堆区当前结束地址
    uint32_t start_stack;            // 栈区起始地址
    uint32_t end_stack;              // 栈区结束地址
    uint32_t total_vm;               // 总虚拟内存大小(页数)
    uint32_t locked_vm;              // 锁定的虚拟内存大小(页数)
    MemoryArea areas[MAX_MEMORY_AREAS]; // 内存区域数组
    uint32_t num_areas;              // 当前内存区域数量
};