#pragma once

#include <arch/x86/gdt.h>
#include <arch/x86/atomic.h>

// 声明per_cpu_offset数组
// extern unsigned long __per_cpu_offset[MAX_CPUS];

unsigned long get_cpu_id();

// // 定义per-cpu变量宏
// #define DEFINE_PER_CPU(type, name) \
//     type name
//
// // 获取当前CPU的per-cpu变量指针
// #define get_cpu_var(name) \
//     ({ \
//         typeof(name)* __ptr = (typeof(name)*)(__per_cpu_offset[get_cpu_id()] + (unsigned long)(&name)); \
//         __ptr; \
//     })
//
// // 获取当前CPU的per-cpu变量引用
// #define get_cpu_ptr(name) \
//     ({ \
//         typeof(&(name)) __ptr = (typeof(&(name)))(__per_cpu_offset[get_cpu_id()] + (unsigned long)(&name)); \
//         __ptr; \
//     })
//
// // 获取其他CPU的per-cpu变量指针
// #define get_cpu_var_smp(cpu, name) \
//     ({ \
//         typeof(name)* __ptr = (typeof(name)*)(__per_cpu_offset[cpu] + (unsigned long)(&name)); \
//         __ptr; \
//     })

void print_pointer(void *ptr);
namespace arch {


// 初始化每CPU数据区
void cpu_init_percpu();

// 获取当前CPU ID
unsigned int get_cpu_id();

template<typename T>
class PerCPU {
public:
    void init(T* data) { data_[get_cpu_id()] = data; }
    void init_all()
    {
        for (unsigned int i = 0; i < MAX_CPUS; i++) {
            data_[i] = new T();
            print_pointer(data_[i]);
        }
    }

    T* operator->() {
        return data_[get_cpu_id()];
    }
    void set(T *t)
    {
        data_[get_cpu_id()] = t;
    }
    T& operator*() {
        return *data_[get_cpu_id()];
    }
    T* get_for_cpu(unsigned int cpu) {
        return data_[cpu];
    }

private:
    T* data_[MAX_CPUS] = {nullptr};
};

} // namespace arch