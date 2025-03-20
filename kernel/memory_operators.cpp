#include "buddy_allocator.h"
#include <stddef.h>
#include <lib/debug.h>

// 全局new操作符实现
void* operator new(size_t size) {
    debug_debug("operator new called with size: %d\n", size);
    uint32_t pages = (size + 4095) / 4096; // 向上取整到页大小
    return (void*)BuddyAllocator::allocate_pages(pages);
}

// 全局new[]操作符实现
void* operator new[](size_t size) {
    debug_debug("operator new[] called with size: %d\n", size);
    uint32_t pages = (size + 4095) / 4096; // 向上取整到页大小
    return (void*)BuddyAllocator::allocate_pages(pages);
}

// 全局delete操作符实现
void operator delete(void* ptr) noexcept {
    if (ptr) {
        BuddyAllocator::free_pages((uint32_t)ptr, 1); // 假设至少分配1页
    }
}

// 全局delete[]操作符实现
void operator delete[](void* ptr) noexcept {
    if (ptr) {
        BuddyAllocator::free_pages((uint32_t)ptr, 1); // 假设至少分配1页
    }
}

// C++14版本的sized delete操作符
void operator delete(void* ptr, size_t size) noexcept {
    if (ptr) {
        uint32_t pages = (size + 4095) / 4096; // 向上取整到页大小
        BuddyAllocator::free_pages((uint32_t)ptr, pages);
    }
}

// C++14版本的sized delete[]操作符
void operator delete[](void* ptr, size_t size) noexcept {
    if (ptr) {
        uint32_t pages = (size + 4095) / 4096; // 向上取整到页大小
        BuddyAllocator::free_pages((uint32_t)ptr, pages);
    }
}