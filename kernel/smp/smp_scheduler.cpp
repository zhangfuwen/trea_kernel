#include <kernel/smp_scheduler.h>
#include <arch/x86/apic.h>
#include <lib/debug.h>
#include "kernel/process.h"

namespace kernel {

DEFINE_PER_CPU(RunQueue, scheduler_runqueue);

void SMP_Scheduler::init() {
    for_each_cpu(cpu) {
        RunQueue* rq = get_cpu_ptr(scheduler_runqueue);
        rq->lock = SPINLOCK_INIT;
        rq->nr_running = 0;
        INIT_LIST_HEAD(&rq->runnable_list);
    }
}

Process* SMP_Scheduler::pick_next_task() {
    RunQueue* rq = get_cpu_ptr(scheduler_runqueue);
    spin_lock(&rq->lock);
    
    if (list_empty(&rq->runnable_list)) {
        spin_unlock(&rq->lock);
        return load_balance(); // 执行负载均衡
    }
    
    Process* next = list_entry(rq->runnable_list.next, Process, sched_list);
    list_del_init(&next->sched_list);
    rq->nr_running--;
    
    spin_unlock(&rq->lock);
    return next;
}

void SMP_Scheduler::enqueue_task(Process* p) {
    uint32_t cpu = p->affinity % apic_get_cpu_count();
    RunQueue* rq = get_cpu_ptr(scheduler_runqueue, cpu);
    
    spin_lock(&rq->lock);
    list_add_tail(&p->sched_list, &rq->runnable_list);
    rq->nr_running++;
    spin_unlock(&rq->lock);
}

Process* SMP_Scheduler::load_balance() {
    // 负载均衡算法：从最繁忙的运行队列窃取任务
    uint32_t busiest_cpu = find_busiest_cpu();
    RunQueue* busiest_rq = get_cpu_ptr(scheduler_runqueue, busiest_cpu);
    
    spin_lock(&busiest_rq->lock);
    if (!list_empty(&busiest_rq->runnable_list)) {
        Process* stolen = list_entry(busiest_rq->runnable_list.next, Process, sched_list);
        list_del_init(&stolen->sched_list);
        busiest_rq->nr_running--;
        spin_unlock(&busiest_rq->lock);
        return stolen;
    }
    spin_unlock(&busiest_rq->lock);
    return nullptr;
}

// 查找最繁忙的CPU
uint32_t SMP_Scheduler::find_busiest_cpu() {
    uint32_t max_tasks = 0;
    uint32_t busiest_cpu = 0;
    
    for_each_cpu(cpu) {
        RunQueue* rq = get_cpu_ptr(scheduler_runqueue, cpu);
        if (rq->nr_running > max_tasks) {
            max_tasks = rq->nr_running;
            busiest_cpu = cpu;
        }
    }
    
    return busiest_cpu;
}

// 设置进程的CPU亲和性
void SMP_Scheduler::set_affinity(Process* p, uint32_t cpu_mask) {
    if (!p) return;
    
    // 确保CPU掩码有效
    uint32_t max_cpus = arch::smp_get_cpu_count();
    uint32_t valid_mask = (1 << max_cpus) - 1;
    cpu_mask &= valid_mask;
    
    // 如果掩码为0，设置为所有CPU
    if (cpu_mask == 0) {
        cpu_mask = valid_mask;
    }
    
    // 更新进程的CPU亲和性
    p->affinity = cpu_mask;
    
    debug_debug("Set process %d affinity to 0x%x\n", p->pid, cpu_mask);
}

// 获取当前CPU的运行队列
RunQueue* SMP_Scheduler::get_current_runqueue() {
    return get_cpu_var(scheduler_runqueue);
}

} // namespace kernel