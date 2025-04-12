#include "arch/x86/cpu.h"
#include "arch/x86/apic.h"
#include "arch/x86/gdt.h"
#include "arch/x86/idt.h"
#include "lib/debug.h"

#include <kernel/kernel.h>

namespace arch {

void cpu_init_percpu() {
    // 初始化CPU本地存储
    debug_debug("Initializing CPU %d...\n", apic_get_id());
    
    // 初始化GDT
    GDT::init();
    
    // 初始化IDT
    IDT::init();

    // 启用本地APIC
    apic_enable();
    
    debug_debug("CPU %d initialized\n", apic_get_id());
}

} // namespace arch