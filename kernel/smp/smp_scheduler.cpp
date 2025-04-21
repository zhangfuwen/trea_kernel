#include <arch/x86/apic.h>
#include <arch/x86/percpu.h>
#include <kernel/kernel.h>
#include <kernel/process.h>
#include <kernel/smp_scheduler.h>
#include <lib/debug.h>

void print_pointer(void *ptr) {
    debug_debug("Pointer value: 0x%x\n", ptr);
}
extern "C" Task* create_idle_task(Context *context, uint32_t lapic_id);
namespace kernel {
// 用PerCPU模板定义每CPU运行队列

void RunQueue::print_list()
{
    debug_debug("RunQueue 0x%x, task_count:%d\n", this, nr_running);
    int i = 0;
    list_for_each(entry, &runnable_list) {
        Task* task = list_entry(entry, Task, sched_list);
        debug_debug("Task(nr:%d) ID: %d\n", i, task->task_id);
        i++;
    }
}



void SMP_Scheduler::init() {
    // scheduler_runqueue.init_all(new RunQueue());

    for (unsigned int cpu = 0; cpu < arch::apic_get_cpu_count(); cpu++) {
        scheduler_runqueue.set(cpu, new RunQueue());
        RunQueue* rq = scheduler_runqueue.get_for_cpu(cpu);
        debug_debug("Initializing SMP scheduler for CPU %d, rq: 0x%x\n", cpu, rq);
        rq->lock = SPINLOCK_INIT;
        rq->nr_running = 0;
        INIT_LIST_HEAD(&rq->runnable_list);
        if(cpu !=0 ) {
            auto task = create_idle_task(ProcessManager::kernel_context, cpu);
            idle_task.set(cpu, task);
            current_task.set(cpu, task);
        }
    }
}

Task* SMP_Scheduler::pick_next_task() {
    RunQueue* rq = scheduler_runqueue.operator->();
    if (rq->nr_running != 0) {
        // debug_debug("Picking next task on CPU %d, rq: 0x%x, task_count:%d\n", arch::apic_get_id(), rq, rq->nr_running);
    }
    // rq->print_list();
    spin_lock(&rq->lock);
    if (list_empty(&rq->runnable_list)) {
        spin_unlock(&rq->lock);
        return load_balance();
    }
    Task* next = list_entry(rq->runnable_list.next, Task, sched_list);
    list_del_init(&next->sched_list);
    rq->nr_running--;
    spin_unlock(&rq->lock);
    // debug_debug("Picked task %d(0x%x) on CPU %d, rq: 0x%x\n", next->task_id, next, arch::apic_get_id(), rq);
    auto cpu = arch::apic_get_id();
    if (cpu != next->cpu) {
        debug_debug("stolen from cpu %d to cpu %d, rq:0x%x\n", next->cpu, cpu, rq);
    }

    return next;
}
void SMP_Scheduler::enqueue_task(Task* p, int cpu_id)
{
    RunQueue* rq = scheduler_runqueue.get_for_cpu(cpu_id);
    // debug_debug("Enqueueing task %d(0x%x) on CPU %d, rq: 0x%x\n", p->task_id, p, cpu_id, rq);
    spin_lock(&rq->lock);
    list_add_tail(&p->sched_list, &rq->runnable_list);
    rq->nr_running++;
    spin_unlock(&rq->lock);
}

void SMP_Scheduler::enqueue_task(Task* p) {
    RunQueue* rq = scheduler_runqueue.operator->();
    // debug_debug("Enqueueing task %d on CPU %d, rq: 0x%x\n", p->task_id, arch::apic_get_id(), rq);
    spin_lock(&rq->lock);
    list_add_tail(&p->sched_list, &rq->runnable_list);
    rq->nr_running++;
    spin_unlock(&rq->lock);
}

Task* SMP_Scheduler::load_balance() {
    RunQueue* rq = scheduler_runqueue.operator->();
    spin_lock(&rq->lock);
    if (!list_empty(&rq->runnable_list)) {
        auto* stolen = list_entry(rq->runnable_list.next, Task, sched_list);
        list_del_init(&stolen->sched_list);
        rq->nr_running--;
        spin_unlock(&rq->lock);
        debug_debug("stealing task %d(0x%x) from CPU %d\n", stolen->task_id, stolen, arch::apic_get_id());
        return stolen;
    }
    spin_unlock(&rq->lock);
    auto idle = get_idle_task();
    // debug_debug("No tasks to run, returning idle task %d\n", idle->task_id);
    return idle;
}

uint32_t SMP_Scheduler::find_busiest_cpu() {
    uint32_t max_tasks = 0;
    uint32_t busiest_cpu = 0;
    for (unsigned int cpu = 0; cpu < MAX_CPUS; ++cpu) {
        RunQueue* rq = scheduler_runqueue.get_for_cpu(cpu);
        if (rq->nr_running > max_tasks) {
            max_tasks = rq->nr_running;
            busiest_cpu = cpu;
        }
    }
    return busiest_cpu;
}

void SMP_Scheduler::set_affinity(Task* p, uint32_t cpu_mask) {
    if (!p) return;
    uint32_t max_cpus = MAX_CPUS;
    uint32_t valid_mask = (1 << max_cpus) - 1;
    cpu_mask &= valid_mask;
    if (cpu_mask == 0) {
        cpu_mask = valid_mask;
    }
    p->affinity = cpu_mask;
    debug_debug("Set process %d affinity to 0x%x\n", p->task_id, cpu_mask);
}

RunQueue* SMP_Scheduler::get_current_runqueue() {
    return scheduler_runqueue.operator->();
}
Task *SMP_Scheduler::get_idle_task()
{
    return idle_task.operator->();
}
void SMP_Scheduler::set_idle_task(Task *task)
{
    idle_task.set(task);
}

Task *SMP_Scheduler::get_current_task()
{
    return current_task.operator->();
}
void SMP_Scheduler::set_current_task(Task *task)
{
    current_task.set(task);
}

} // namespace kernel