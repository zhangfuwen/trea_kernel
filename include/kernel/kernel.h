#pragma once
#include <stdint.h>
#include "kernel/kernel_memory.h"

// 内核管理类（单例）
class Kernel {
public:
    static Kernel instance0;
    // 获取单例实例
    static Kernel& instance() {
        return instance0;
    }

    KernelMemory& kernel_mm() {
        return memory_manager;
    }

    // 初始化内核
    void init();

    static void init_all() {
        // instance0 = new Kernel();
    }
private:
    // 私有构造函数（单例模式）
    Kernel();
    Kernel(const Kernel&) = delete;
    Kernel& operator=(const Kernel&) = delete;

    // 内核内存管理器
    KernelMemory memory_manager;
};