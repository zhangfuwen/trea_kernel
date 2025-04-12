#include <arch/x86/apic.h>
#include <arch/x86/interrupt.h>
#include <arch/x86/percpu.h>
#include <arch/x86/smp.h>
#include <kernel/kernel.h>
#include <kernel/scheduler.h>
#include <lib/debug.h>

namespace arch
{

static uint32_t bsp_lapic_id = 0;
volatile uint32_t cpu_ready_count = 0;

void smp_init()
{
    // 初始化BSP(Bootstrap Processor)的LAPIC
    // 获取当前CPU(BSP)的LAPIC ID作为主处理器标识
    bsp_lapic_id = apic_get_id();
    auto cpu_count = apic_get_cpu_count();
    debug_debug("系统总CPU数量: %d, BSP ID: %d\n", cpu_count, bsp_lapic_id);

    // 发送INIT-SIPI-SIPI序列启动APs(Application Processors)
    // 1. 发送INIT重置AP
    // 2. 发送两次SIPI(Startup IPI)启动AP，AP将从物理地址0x8000开始执行
    // 3. AP执行位于0x8000处的启动代码后跳转到ap_entry继续初始化
    for(uint32_t i = 0; i < cpu_count; i++) {
        debug_debug("CPU %d 正在启动\n", i);
        uint32_t target_id = i;
        if(target_id != bsp_lapic_id) {
            apic_send_init(target_id);       // 发送INIT重置AP
            for(int j = 0; j < 100000; j++) { // 等待AP重置完成
                __asm__ volatile("pause");
            }
            apic_send_sipi(0x8000, target_id); // 第一次SIPI，AP从物理地址0x8000开始执行
            for(int j = 0; j < 100000; j++) { // 等待AP重置完成
                __asm__ volatile("pause");
            }
            apic_send_sipi(0x8000, target_id); // 第二次SIPI，确保AP能收到启动信号
            for(int j = 0; j < 100000; j++) { // 等待AP重置完成
                __asm__ volatile("pause");
            }
        }
    }

    // 等待APs完成初始化
    while(cpu_ready_count < apic_get_cpu_count()) {
        __asm__ volatile("pause");
    }
}

void ap_entry()
{
    // AP从0x8000的启动代码跳转到这里继续执行
    // 初始化当前AP的LAPIC，使其能够接收中断
    debug_debug("ap_entry\n");
    apic_init();
    uint32_t current_cpu_id = apic_get_id();
    debug_debug("CPU %d 已启动\n", current_cpu_id);

    // 初始化本地APIC定时器
    apic_init_timer(100);

    // 初始化GDT、IDT和CPU本地存储
    cpu_init_percpu();

    // 加载BSP的页表
    uint32_t bsp_cr3;
    asm volatile("movl %%cr3, %0" : "=r"(bsp_cr3));
    asm volatile("movl %0, %%cr3" : : "r"(bsp_cr3));

    // 初始化SMP调度器
    auto& kernel = Kernel::instance();

    // 启用中断
    asm volatile("sti");

    // 原子增加就绪计数器
    __atomic_add_fetch(&cpu_ready_count, 1, __ATOMIC_SEQ_CST);

    // 进入调度循环
    while(true) {
        uint32_t current_cpu_id = apic_get_id();
        debug_debug("CPU %d 正在调度任务\n", current_cpu_id);
        kernel.scheduler().pick_next_task();
        asm volatile("hlt");
    }
}

uint32_t smp_get_cpu_count()
{
    return apic_get_cpu_count();
}

} // namespace arch