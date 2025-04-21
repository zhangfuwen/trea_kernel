#include "../include/kernel/kernel.h"
#include <../include/lib/debug.h>
#include <stddef.h>

// 全局new操作符实现
void* operator new(size_t size)
{
    log_debug("operator new called with size: %d\n", size);
    auto ret = Kernel::instance().kernel_mm().kmalloc(size);
    log_debug("operator new called with size: %d, ret: %p\n", size, ret);
    return ret;
}

// 全局new[]操作符实现
void* operator new[](size_t size)
{
    log_debug("operator new[] called with size: %d\n", size);
    auto ret = Kernel::instance().kernel_mm().kmalloc(size);
    log_debug("operator new called with size: %d, ret: %p\n", size, ret);
    return ret;
}

// In-place new operator
void* operator new(size_t size, void* ptr) noexcept
{
    return ptr;
}

// In-place new[] operator
void* operator new[](size_t size, void* ptr) noexcept
{
    return ptr;
}

// 全局delete操作符实现
void operator delete(void* ptr) noexcept
{
    log_debug("operator delete called with ptr: %p\n", ptr);
    if(ptr) {
        Kernel::instance().kernel_mm().kfree(ptr);
    }
}

// 全局delete[]操作符实现
void operator delete[](void* ptr) noexcept
{
    log_debug("operator delete called with ptr: %p\n", ptr);
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