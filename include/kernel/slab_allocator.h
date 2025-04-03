#pragma once

#include <stddef.h>
#include <stdint.h>

namespace kernel {

// Slab对象描述符
struct SlabObject {
    SlabObject* next;  // 空闲对象链表的下一个节点
    void print() const;
};

// Slab页描述符
struct SlabCache;
struct Slab {
    size_t inuse;      // 已使用的对象数量
    size_t free;       // 空闲对象数量
    SlabObject* freelist;  // 空闲对象链表
    void* objects;     // 对象数组的起始地址
    Slab* next;        // 链表中的下一个slab
    SlabCache* cache;  // 指向所属的SlabCache
    void print() const;
};

// Slab缓存
class SlabCache {
public:
    SlabCache();
    SlabCache(const char* name, size_t size, size_t align = 8);
    ~SlabCache();

    // 分配和释放对象
    void* alloc();
    void free(void* ptr);

    // 创建和销毁slab
    Slab* create_slab();
    void destroy_slab(Slab* slab);

    // 打印缓存信息
    void print() const;

private:
    char name[32];      // 缓存名称
    size_t object_size;    // 对象大小
    size_t object_align;   // 对象对齐要求
    size_t objects_per_slab;  // 每个slab中的对象数量

    Slab* slabs_full;      // 完全使用的slab链表
    Slab* slabs_partial;   // 部分使用的slab链表
    Slab* slabs_free;      // 完全空闲的slab链表
};

// Slab分配器
class SlabAllocator {
public:
    void init();
    void* kmalloc(size_t size);
    void kfree(void* ptr);
    SlabAllocator();
    ~SlabAllocator();

private:

    // 通用缓存数组，支持8字节到4KB的对象
    static constexpr size_t NUM_GENERAL_CACHES = 9;
    SlabCache *general_caches[NUM_GENERAL_CACHES];
    SlabCache _general_caches[NUM_GENERAL_CACHES]; // memory

    // 获取合适大小的通用缓存
    SlabCache* get_general_cache(size_t size);
};

} // namespace kernel