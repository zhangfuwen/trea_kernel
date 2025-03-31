#include "kernel/kernel.h"
#include <lib/debug.h>
#include <stddef.h>

// 全局new操作符实现
void* operator new(size_t size)
{
    debug_debug("operator new called with size: %d\n", size);
    return Kernel::instance().kernel_mm().kmalloc(size);
}

// 全局new[]操作符实现
void* operator new[](size_t size)
{
    debug_debug("operator new[] called with size: %d\n", size);
    return Kernel::instance().kernel_mm().kmalloc(size);
}

// 全局delete操作符实现
void operator delete(void* ptr) noexcept
{
    if(ptr) {
        Kernel::instance().kernel_mm().kfree(ptr);
    }
}

// 全局delete[]操作符实现
void operator delete[](void* ptr) noexcept
{
    if(ptr) {
        Kernel::instance().kernel_mm().kfree(ptr);
    }
}

// C++14版本的sized delete操作符
void operator delete(void* ptr, size_t size) noexcept
{
    if(ptr) {
        Kernel::instance().kernel_mm().kfree(ptr);
    }
}

// C++14版本的sized delete[]操作符
void operator delete[](void* ptr, size_t size) noexcept
{
    if(ptr) {
        Kernel::instance().kernel_mm().kfree(ptr);
    }
}