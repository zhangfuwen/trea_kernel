#pragma once
#include <stdint.h>

#include "kernel/kernel_memory.h"
#include "user_memory.h"

extern "C" void page_fault_handler(uint32_t error_code, uint32_t fault_addr);
// 内核管理类（单例）
class Kernel
{
public:
    static Kernel instance0;
    // 获取单例实例
    static Kernel& instance()
    {
        return instance0;
    }

    KernelMemory& kernel_mm()
    {
        return memory_manager;
    }

    // 初始化内核
    void init();

    // 检查当前CPU特权级别
    static bool is_kernel_mode();

    static void init_all()
    {
        // instance0 = new Kernel();
    }

    inline void tick() { timer_ticks++;}
    uint32_t get_ticks()
    {
        return timer_ticks;
    }

private:
    // 私有构造函数（单例模式）
    Kernel();
    Kernel(const Kernel&) = delete;
    Kernel& operator=(const Kernel&) = delete;

    // 内核内存管理器
    KernelMemory memory_manager;

    uint32_t timer_ticks;
};