#include "arch/x86/interrupt.h"
#include "lib/debug.h"

extern "C" void general_fault_errno_handler(uint32_t isr_no, uint32_t error_code)
{
    debug_debug("fault occurred! NO. %d, Error code: %d\n", isr_no, error_code);
}
extern "C" void segmentation_fault_handler(uint32_t error_code)
{
    debug_debug("Segment fault occurred! Error code: %d\n", error_code);
    // 根据错误码分析具体原因
    if(error_code & 0x1) {
        debug_debug("External event (not caused by program)\n");
    }
    if(error_code & 0x2) {
        debug_debug("IDT gate type fault\n");
    }
    if(error_code & 0x4) {
        debug_debug("LDT or IDT fault\n");
    }
    if(error_code & 0x8) {
        debug_debug("Segment not present\n");
    }
    // 暂时直接停止系统
    while(1) {
        asm("hlt");
    }
}