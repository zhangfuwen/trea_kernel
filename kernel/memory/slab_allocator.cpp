#include <arch/x86/paging.h>
#include <kernel/buddy_allocator.h>
#include <kernel/kernel.h>
#include <kernel/slab_allocator.h>
#include <lib/debug.h>
#include <lib/string.h>
#include <arch/x86/spinlock.h>

namespace kernel {

static SpinLock slab_global_lock;

class Locker
{
    public:
    Locker() { slab_global_lock.acquire(); }
    ~Locker() { slab_global_lock.release(); }
};

void SlabObject::print() const {
    debug_info("SlabObject at %p, next=%p\n", this, next);
}

void Slab::print() const {
    debug_info("Slab at %p:\n", this);
    debug_info("  inuse=%d, free=%d\n", inuse, free);
    debug_info("  freelist=%p, objects=%p, next=%p\n", freelist, objects, next);
    
    // 打印空闲对象链表
    debug_info("  Free objects: \n");
    SlabObject* obj = freelist;
    while (obj) {
        obj = obj->next;
    }
}

void SlabCache::print() const {
    debug_info("SlabCache '%s':\n", name);
    debug_info("  object_size=%d, object_align=%d, objects_per_slab=%d\n",
              object_size, object_align, objects_per_slab);
    
    // 打印完全使用的slabs
    debug_info("  Full slabs: ");
    Slab* slab = slabs_full;
    while (slab) {
        debug_info("%p -> ", slab);
        slab = slab->next;
    }
    debug_info("null\n");
    
    // 打印部分使用的slabs
    debug_info("  Partial slabs: ");
    slab = slabs_partial;
    while (slab) {
        debug_info("%p -> ", slab);
        slab = slab->next;
    }
    debug_info("null\n");
    
    // 打印空闲的slabs
    debug_info("  Free slabs: ");
    slab = slabs_free;
    while (slab) {
        debug_info("%p -> ", slab);
        slab = slab->next;
    }
    debug_info("null\n");
}

/**
 * @brief 默认构造函数，创建一个空的Slab缓存
 */
SlabCache::SlabCache()
    : name("---"), object_size(0), object_align(0), objects_per_slab(0), slabs_full(nullptr),
      slabs_partial(nullptr), slabs_free(nullptr)
{
}

/**
 * @brief 构造函数，创建一个指定名称、大小和对齐要求的Slab缓存
 * @param name 缓存名称
 * @param size 对象大小
 * @param align 对象对齐要求
 */
SlabCache::SlabCache(const char* name, size_t size, size_t align)
    : object_size(size)
    , object_align(align)
    , slabs_full(nullptr)
    , slabs_partial(nullptr)
    , slabs_free(nullptr)
{
    strcpy(this->name, name);
    // 计算每个slab中可以容纳的对象数量
    size_t page_size = PAGE_SIZE;
    size_t available = page_size - sizeof(Slab);
    debug_info("SlabCache('%s') PAGE_SIZE=%d sizeof(Slab)=%d available=%d object_size=%d\n", name, page_size, sizeof(Slab), available, object_size);
    objects_per_slab = available / object_size;
    debug_info("Created slab cache '%s' with object size %d, align %d, objects per slab %d\n",
              name, size, align, objects_per_slab);
}

/**
 * @brief 析构函数，释放所有分配的slab
 */
SlabCache::~SlabCache()
{
    // 释放所有slab
    Slab* slab;
    while ((slab = slabs_full)) {
        slabs_full = slab->next;
        destroy_slab(slab);
    }
    while ((slab = slabs_partial)) {
        slabs_partial = slab->next;
        destroy_slab(slab);
    }
    while ((slab = slabs_free)) {
        slabs_free = slab->next;
        destroy_slab(slab);
    }
    debug_info("Destroyed slab cache '%s'\n", name);
}

/**
 * @brief 从Slab缓存中分配一个对象
 * @return 分配的对象指针，如果分配失败则返回nullptr
 */
void* SlabCache::alloc()
{
    Slab* slab = slabs_partial ? slabs_partial : slabs_free;
    if (!slab) {
        // 没有可用的slab，创建新的
        slab = create_slab();
        if (!slab) {
            debug_debug("Failed to create new slab in cache '%s'\n", name);
            return nullptr;
        }
        slabs_free = slab;
        debug_info("Created new slab in cache '%s'\n", name);
    }

    // 从空闲链表中获取一个对象
    SlabObject* obj = slab->freelist;
    slab->freelist = obj->next;
    slab->inuse++;
    slab->free--;

    // 更新slab状态
    if (slab->free == 0) {
        // 将slab从partial/free链表移到full链表
        if (slab == slabs_partial)
            slabs_partial = slab->next;
        else
            slabs_free = slab->next;
        slab->next = slabs_full;
        slabs_full = slab;
        debug_debug("Slab in cache '%s' became full\n", name);
    } else if (slab == slabs_free) {
        // 将slab从free链表移到partial链表
        slabs_free = slab->next;
        slab->next = slabs_partial;
        slabs_partial = slab;
        debug_debug("Slab in cache '%s' became partial\n", name);
    }
    if (obj == nullptr) {
        debug_err("Failed to allocate object from slab in cache '%s'\n", name);
        slab->print();
        return nullptr;
    }

    debug_debug("Allocated object %p from cache '%s'\n", obj, name);
    return obj;
}

/**
 * @brief 释放一个对象回Slab缓存
 * @param ptr 要释放的对象指针
 */
void SlabCache::free(void* ptr)
{
    // 获取对象所在的slab
    Slab* slab = (Slab*)((uintptr_t)ptr & ~(PAGE_SIZE - 1));
    
    // 将对象添加到空闲链表
    SlabObject* obj = (SlabObject*)ptr;
    obj->next = slab->freelist;
    slab->freelist = obj;
    slab->inuse--;
    slab->free++;

    // 更新slab状态
    if (slab->inuse == 0) {
        // 将slab从full/partial链表移到free链表
        if (slab == slabs_full)
            slabs_full = slab->next;
        else {
            Slab** prev = &slabs_partial;
            while (*prev && *prev != slab)
                prev = &(*prev)->next;
            if (*prev)
                *prev = slab->next;
        }
        slab->next = slabs_free;
        slabs_free = slab;
        debug_debug("Slab in cache '%s' became free\n", name);
    } else if (slab->free == 1) {
        // 将slab从full链表移到partial链表
        Slab** prev = &slabs_full;
        while (*prev && *prev != slab)
            prev = &(*prev)->next;
        if (*prev) {
            *prev = slab->next;
            slab->next = slabs_partial;
            slabs_partial = slab;
            debug_debug("Slab in cache '%s' became partial\n", name);
        }
    }

    debug_debug("Freed object %p back to cache '%s'\n", ptr, name);
}

/**
 * @brief 创建一个新的slab
 * @return 创建的slab指针，如果创建失败则返回nullptr
 */
Slab* SlabCache::create_slab()
{
    // 分配一个页面
    auto &paging = Kernel::instance().kernel_mm().paging();
    PADDR pa = Kernel::instance().kernel_mm().alloc_pages(0, 0); // order=0表示分配单个页面
    void * page = Kernel::instance().kernel_mm().phys2Virt(pa);
    if (!page) {
        debug_err("Failed to allocate page for new slab in cache '%s'\n", name);
        return nullptr;
    }

    // 初始化slab描述符
    Slab* slab = (Slab*)page;
    if(slab == (Slab*)0xc17cf000) {
        printPDPTE((VADDR)slab);
        printPD(paging.getCurrentPageDirectory(), 0, 0x400);

    }
    slab->inuse = 0;
    slab->free = objects_per_slab;
    auto tmp = page + sizeof(Slab);
    slab->next = nullptr;
    slab->cache = this;  // 设置指向所属的SlabCache的指针
    slab->objects = tmp;
    debug_debug("debug ----0x%x\n", slab);
    slab->objects = tmp;
    debug_debug("debug objects ---- 0x%x, 0x%x\n", tmp,slab->objects);
    debug_debug("debug page ----0x%x\n", page);
    debug_debug("debug sizeof(Slab) ----%d\n", sizeof(Slab));

    // 初始化空闲对象链表
    char* obj = (char*)slab->objects;
    if(obj == nullptr) {
        debug_debug("page: %p, slab: %p, slab->objects: %p\n", page, slab, slab->objects);
    }
    slab->freelist = (SlabObject*)obj;
    if(slab->freelist == nullptr) {
        debug_debug("Failed to allocate page for new slab in cache '%s'\n", name);
        return nullptr;
    }
    for (size_t i = 0; i < objects_per_slab - 1; i++) {
        ((SlabObject*)obj)->next = (SlabObject*)(obj + object_size);
        obj += object_size;
    }
    ((SlabObject*)obj)->next = nullptr;

    debug_debug("Created new slab at %p in cache '%s'\n", slab, name);
    return slab;
}

/**
 * @brief 销毁一个slab
 * @param slab 要销毁的slab指针
 */
void SlabCache::destroy_slab(Slab* slab)
{
    debug_debug("Destroying slab at %p in cache '%s'\n", slab, name);
    auto va = (void *)slab;
    auto pa = Kernel::instance().kernel_mm().virt2Phys(va);
    Kernel::instance().kernel_mm().free_pages(pa, 0); // order=0表示释放单个页面
}

/**
 * @brief 构造函数，初始化通用缓存数组
 */
SlabAllocator::SlabAllocator()
{
    // 初始化通用缓存
    const size_t sizes[] = {8, 16, 32, 64, 128, 256, 512, 1024, 2048};
    for (size_t i = 0; i < NUM_GENERAL_CACHES; i++) {
        char name[32];
        int len = format_string(name, sizeof(name), "size-%d", sizes[i]);
        name[len] = '\0';
        general_caches[i] = new ((void*)&_general_caches[i]) SlabCache(name, sizes[i]);
    }
}

/**
 * @brief 析构函数
 */
SlabAllocator::~SlabAllocator()
{
    for (size_t i = 0; i < NUM_GENERAL_CACHES; i++) {
        // general_caches[i]->~SlabCache();
    }
}

/**
 * @brief 初始化Slab分配器
 */
void SlabAllocator::init()
{
    const size_t sizes[] = {8, 16, 32, 64, 128, 256, 512, 1024, 2048};
    for (size_t i = 0; i < NUM_GENERAL_CACHES; i++) {
        char name[32];
        format_string(name, sizeof(name), "size-%u", sizes[i]);
        general_caches[i] = new ((void*)&_general_caches[i]) SlabCache(name, sizes[i]);
    }
    debug_info("Initialized slab allocator with %d general caches\n", NUM_GENERAL_CACHES);
}

/**
 * @brief 分配指定大小的内存
 * @param size 要分配的内存大小
 * @return 分配的内存指针，如果分配失败则返回nullptr
 */
void* SlabAllocator::kmalloc(size_t size)
{
    Locker lock;
    if (size == 0) {
        debug_warn("Attempted to allocate 0 bytes\n");
        return nullptr;
    }

    // 对于大于4KB的分配，直接使用页分配器
    if (size > 2048) {
        size_t num_pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
        uint32_t order = 0;
        while ((1U << order) < num_pages) order++;
        auto phys_addr = Kernel::instance().kernel_mm().alloc_pages(0, order);
        if (!phys_addr) {
            debug_err("Failed to allocate %d pages for large allocation\n", num_pages);
            return nullptr;
        }
        debug_info("Allocated %d pages for large allocation of size %d\n", num_pages, size);
        return (void*)Kernel::instance().kernel_mm().phys2Virt(phys_addr);
    }

    // 使用合适的通用缓存
    SlabCache* cache = get_general_cache(size);
    if (!cache) {
        debug_err("Failed to find appropriate cache for size %d\n", size);
        return nullptr;
    }
    void* ret = cache->alloc();
    if (!ret) {
        debug_err("Failed to allocate object of size %d from cache\n", size);
        return nullptr;
    }
    debug_debug("Allocated memory of size %d at %p\n", size, ret);
    return ret;
}

/**
 * @brief 释放之前分配的内存
 * @param ptr 要释放的内存指针
 */
void SlabAllocator::kfree(void* ptr)
{
    Locker lock;
    if (!ptr) {
        debug_warn("Attempted to free null pointer\n");
        return;
    }

    // 检查指针是否在页面边界上，用于区分大内存分配和小对象分配
    if ((uintptr_t)ptr & (PAGE_SIZE - 1)) {
        // 通过将指针地址对齐到页面边界，获取对象所在的slab描述符
        Slab* slab = (Slab*)((uintptr_t)ptr & ~(PAGE_SIZE - 1));
        
        // 获取对象所属的SlabCache
        SlabCache* cache = slab->cache;
        if (!cache) {
            debug_err("Failed to find cache for object at %p\n", ptr);
            return;
        }
        cache->free(ptr);
        debug_debug("Freed small object at %p\n", ptr);
    } else {
        // 对于大内存分配，直接通过页分配器释放
        PADDR pa = Kernel::instance().kernel_mm().virt2Phys(ptr);
        Kernel::instance().kernel_mm().free_pages(pa, 0); // order=0表示释放单个页面
        debug_debug("Freed large object at %p\n", ptr);
    }
}

/**
 * @brief 获取适合指定大小的通用缓存
 * @param size 对象大小
 * @return 合适的缓存指针，如果没有合适的缓存则返回nullptr
 */
SlabCache* SlabAllocator::get_general_cache(size_t size)
{
    const size_t sizes[] = {8, 16, 32, 64, 128, 256, 512, 1024, 2048};
    for (size_t i = 0; i < NUM_GENERAL_CACHES; i++) {
        if (size <= sizes[i]) {
            return general_caches[i];
        }
    }
    debug_err("No appropriate cache found for size %d\n", size);
    return nullptr;
}

} // namespace kernel