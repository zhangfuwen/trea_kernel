#pragma once

#include <kernel/scheduler.h>
#include <kernel/process.h>
#include <arch/x86/spinlock.h>
#include <arch/x86/percpu.h>
#include <arch/x86/smp.h>
#include <kernel/list.h>

namespace kernel {

// 每CPU运行队列结构
struct RunQueue {
    SpinLock lock;           // 运行队列锁
    uint32_t nr_running;     // 可运行进程数量
    struct list_head runnable_list;  // 可运行进程链表
};

// 定义每CPU运行队列
#define SPINLOCK_INIT SpinLock()

// SMP调度器类
class SMP_Scheduler {
public:
    // 初始化SMP调度器
    void init();
    
    // 选择下一个要运行的任务
    Task* pick_next_task();
    
    // 将任务加入运行队列
    void enqueue_task(Task* p);
    
    // 执行负载均衡
    Task* load_balance();
    
    // 查找最繁忙的CPU
    uint32_t find_busiest_cpu();
    
    // 设置进程的CPU亲和性
    void set_affinity(Task* p, uint32_t cpu_mask);
    
    // 获取当前CPU的运行队列
    RunQueue* get_current_runqueue();
};

// 遍历所有CPU的宏
#define for_each_cpu(cpu) \
    for (uint32_t cpu = 0; cpu < arch::smp_get_cpu_count(); cpu++)

} // namespace kernel