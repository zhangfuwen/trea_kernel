#include <arch/x86/apic.h>
#include <arch/x86/interrupt.h>
#include <arch/x86/percpu.h>
#include <arch/x86/smp.h>
#include <kernel/kernel.h>
#include <kernel/scheduler.h>
#include <lib/debug.h>
#include <lib/serial.h>

namespace arch
{

static uint32_t bsp_lapic_id = 0;
volatile uint32_t cpu_ready_count = 0;

void smp_init()
{
    // 初始化 BSP (Bootstrap Processor) 的 LAPIC
    // 获取当前 CPU (BSP) 的 LAPIC ID 作为主处理器标识
    bsp_lapic_id = apic_get_id();
    auto cpu_count = apic_get_cpu_count();
    log_debug("系统总 CPU 数量: %d, BSP ID: %d\n", cpu_count, bsp_lapic_id);

    // 发送 INIT-SIPI-SIPI 序列启动 APs (Application Processors)
    // 1. 发送 INIT 重置 AP
    // 2. 发送两次 SIPI (Startup IPI) 启动 AP，AP 将从物理地址 0x8000 开始执行
    // 3. AP 执行位于 0x8000 处的启动代码后跳转到 ap_entry 继续初始化
    for(uint32_t i = 0; i < cpu_count; i++) {
        uint32_t target_id = i;
        if(target_id != bsp_lapic_id) {
            log_debug("CPU %d 正在启动\n", target_id);
            apic_send_init(target_id);         // 发送 INIT 重置 AP
            for(int j = 0; j < 1000000; j++) { // 短暂等待 AP 重置完成
                __asm__ volatile("pause");
            }
            apic_send_sipi(0x8000, target_id); // 第一次 SIPI，AP 从物理地址 0x8000 开始执行
            for(int j = 0; j < 1000000; j++) { // 短暂等待 AP 启动
                __asm__ volatile("pause");
            }
            apic_send_sipi(0x8000, target_id); // 第二次 SIPI，确保 AP 能收到启动信号
            for(int j = 0; j < 1000000; j++) { // 短暂等待 AP 启动
                __asm__ volatile("pause");
            }
        }
    }

    // 等待所有 APs 完成初始化
    log_debug("等待所有 CPU 就绪...\n");
    while(cpu_ready_count < apic_get_cpu_count() - 1) {
        log_debug("当前就绪 CPU 数量: %d\n", cpu_ready_count);
        for(int j = 0; j < 1000000; j++) { // 短暂等待所有 CPU 就绪
            __asm__ volatile("pause");
        }
        __asm__ volatile("pause");
    }
    log_debug("所有(%d) CPU 已就绪！\n", apic_get_cpu_count());
}


void ap_entry()
{
    // AP 从 0x8000 的启动代码跳转到这里继续执行
    // 初始化当前 AP 的 LAPIC，使其能够接收中断
    uint32_t current_cpu_id = apic_get_id();
    log_debug("ap_entry, CPU ID: %d\n", current_cpu_id);
    apic_init();
    log_debug("CPU %d 已启动\n", current_cpu_id);

    // 初始化本地 APIC 定时器
    apic_init_timer(100);

    // 初始化 GDT、IDT 和 CPU 本地存储
    // 注意：在 SMP 系统中，所有 CPU 共享同一个 IDT
    // IDT 由 BSP 初始化，AP 加载相同的 IDT
    // 这样所有 CPU 使用相同的 interrupt handlers
    cpu_init_percpu();

    auto &kernel = Kernel::instance();
    GDT::loadGDT();
    GDT::loadTR(current_cpu_id);
    kernel.kernel_mm().paging().loadPageDirectory(0x400000);
    kernel.kernel_mm().paging().enablePaging();
    auto task = kernel.scheduler().get_current_task();
    auto cr3 = task->regs.cr3;
    log_debug("cr3: 0x%x, task: %d(0x%x)\n", cr3, task->task_id, task);
    task->print();
    asm volatile("mov %0, %%cr3" ::"r"(cr3));
    log_debug("updating tss, esp0: 0x%x, cr3: 0x%x\n", task->stacks.esp0, cr3);
    GDT::updateTSS(current_cpu_id, task->stacks.esp0, 0x10);
    GDT::updateTSSCR3(current_cpu_id, cr3);
    task->cpu = current_cpu_id;
    // 内核态 -> 内核态中断，不会加载esp0，需要手动加载。
    asm volatile("mov %0, %%esp" ::"r"(task->stacks.esp0));

    // 原子增加就绪计数器
    __atomic_add_fetch(&cpu_ready_count, 1, __ATOMIC_SEQ_CST);
    log_debug("CPU %d 已就绪\n", current_cpu_id);

    for(int j = 0; j < 1000000; j++) { // 短暂等待 AP 启动
        __asm__ volatile("pause");
    }

    // 启用中断
    log_debug("启用中断\n");
    asm volatile("sti");
    // asm volatile("jmp %0" ::"m"(task->regs.eip));

    // // 进入空闲循环，等待中断
    // // 任务调度将在中断处理函数中进行，例如定时器中断
    while(true) {
//        debug_rate_limited("CPU %d 空闲中，等待中断...\n", current_cpu_id);
        asm volatile("hlt");
    }
}

uint32_t smp_get_cpu_count() { return apic_get_cpu_count(); }

} // namespace arch