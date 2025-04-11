#include <arch/x86/smp.h>
#include <arch/x86/apic.h>
#include <arch/x86/interrupt.h>
#include <lib/debug.h>

namespace arch {

static uint32_t bsp_lapic_id = 0;
volatile uint32_t cpu_ready_count = 0;

void smp_init() {
    // 初始化BSP的LAPIC
    bsp_lapic_id = apic_get_id();
    
    // 发送INIT-SIPI-SIPI序列启动APs
    apic_send_init(bsp_lapic_id);
    apic_send_sipi(0x08, bsp_lapic_id);  // 设置CS=0x800
    apic_send_sipi(0x08, bsp_lapic_id);
    
    // 等待APs完成初始化
    while (cpu_ready_count < apic_get_cpu_count()) {
        __asm__ volatile("pause");
    }
}

void ap_entry() {
    // 初始化当前CPU的LAPIC
    apic_init();
    
    // 初始化本地APIC定时器
    apic_init_timer();
    
    // 初始化CPU本地存储
    cpu_init_percpu();
    
    // 原子增加就绪计数器
    __atomic_add_fetch(&cpu_ready_count, 1, __ATOMIC_SEQ_CST);
    
    // 进入调度循环
    for(;;) {
        __asm__ volatile("hlt");
    }
}

} // namespace arch