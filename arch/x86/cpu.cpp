#include "arch/x86/cpu.h"
#include "arch/x86/apic.h"
#include "arch/x86/gdt.h"
#include "arch/x86/idt.h"
#include "lib/debug.h"

#include <kernel/kernel.h>
namespace arch {

unsigned int get_cpu_id() {
    return apic_get_id();
}

void cpu_init_percpu() {
    // 初始化CPU本地存储
    debug_debug("Initializing CPU %d...\n", apic_get_id());
    
    // 初始化GDT
    GDT::loadGDT();
    
    // 初始化IDT
    IDT::loadIDT();

    // for (unsigned int cpu = 0; cpu < MAX_CPUS; cpu++) {
    //     // 分配每CPU数据区
    //     percpu_areas[cpu] = (arch::percpu_area*)Kernel::instance().kernel_mm().kmalloc(percpu_size);
    //
    //
    //     // 设置偏移量
    //     __per_cpu_offset[cpu] = (unsigned long)percpu_areas[cpu];
    // }
    //
    // 启用本地APIC
    apic_enable();
    
    debug_debug("CPU %d initialized\n", apic_get_id());
}

} // namespace arch