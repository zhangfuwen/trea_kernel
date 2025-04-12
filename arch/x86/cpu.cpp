#include "arch/x86/cpu.h"
#include "arch/x86/apic.h"
#include "lib/debug.h"

namespace arch {

void cpu_init_percpu() {
    // 初始化CPU本地存储
    debug_debug("Initializing CPU %d...\n", apic_get_id());
    
    // TODO: 初始化GDT、IDT等CPU本地数据结构
    
    // 启用本地APIC
    apic_enable();
    
    debug_debug("CPU %d initialized\n", apic_get_id());
}

} // namespace arch