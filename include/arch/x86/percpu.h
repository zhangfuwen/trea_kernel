#pragma once

#include <arch/x86/gdt.h>
#include <arch/x86/atomic.h>

#define DEFINE_PER_CPU(type, name) \
    __attribute__((section(".percpu"))) type name

#define get_cpu_var(name) \
    ({ \
        typeof(name)* __ptr; \
        __asm__("mov %%gs:%1, %0" : "=r"(__ptr) : "m"(name)); \
        __ptr; \
    })

#define get_cpu_ptr(name) \
    ({ \
        typeof(&(name)) __ptr; \
        __asm__("mov %%gs:%1, %0" : "=r"(__ptr) : "m"(name)); \
        __ptr; \
    })

namespace arch {

struct percpu_area {
    uint32_t cpu_id;
    void* kernel_stack;
    // 扩展区用于动态分配的每CPU变量
    uint8_t extended_area[4096 - sizeof(uint32_t) - sizeof(void*)];
} __attribute__((aligned(4096)));

extern "C" {
    extern percpu_area __percpu_start[];
    extern percpu_area __percpu_end[];
}

void cpu_init_percpu();

// 模板类封装每CPU变量访问
template<typename T>
class PerCPU {
public:
    PerCPU() = default;

    T* operator->() {
        T* ptr;
        __asm__ volatile("mov %%gs:%1, %0" : "=r"(ptr) : "m"(data));
        return ptr;
    }

    T& operator*() {
        return *operator->();
    }

private:
    DEFINE_PER_CPU(T, data);
};

} // namespace arch