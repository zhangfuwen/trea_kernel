#pragma once

#include <cstdint>

namespace arch {

// SMP相关常量
#define MAX_CPUS 32

// CPU状态标志
enum CpuState {
    CPU_OFFLINE = 0,
    CPU_STARTING,
    CPU_ONLINE,
    CPU_HALTED
};

// CPU信息结构
struct CpuInfo {
    uint32_t id;           // APIC ID
    uint32_t state;        // CPU状态
    void* stack;           // 内核栈
    uint32_t current_pid;  // 当前运行的进程ID
};

// 初始化SMP子系统
void smp_init();

// 获取当前CPU的ID
uint32_t smp_get_current_cpu();

// 获取CPU总数
uint32_t smp_get_cpu_count();

// 获取指定CPU的信息
CpuInfo* smp_get_cpu_info(uint32_t cpu_id);

// AP入口点函数
extern "C" void ap_entry();

// 等待所有CPU初始化完成
void smp_wait_for_aps();

// 在指定CPU上执行函数
void smp_call_function(void (*func)(void*), void* arg, uint32_t cpu_id);

// 在所有CPU上执行函数
void smp_call_function_all(void (*func)(void*), void* arg);

// 外部变量声明
extern volatile uint32_t cpu_ready_count;

} // namespace arch