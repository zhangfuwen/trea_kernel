#include "arch/x86/interrupt.h"
#include "lib/debug.h"

extern "C" void general_fault_errno_handler(uint32_t isr_no, uint32_t error_code)
{
    debug_debug("fault occurred! NO. %d, Error code: %d\n", isr_no, error_code);
    auto pcb = ProcessManager::get_current_process();
    pcb->print();
}

// 新增通用保护故障处理函数
extern "C" void general_protection_fault_handler(uint32_t error_code)
{
    debug_debug("General Protection Fault! Error code: %d\n", error_code);

    // 解析错误代码位
    if(error_code & 0x1) {
        debug_debug("External event (hardware interrupt)\n");
    }
    if(error_code & 0x2) {
        debug_debug("Descriptor type: %s\n", (error_code & 0x4) ? "IDT" : "GDT/LDT");
    }
    if(error_code & 0x8) {
        // 直接打印选择子值（高16位）
        debug_debug("Segment selector out of bounds (0x%x)\n", (error_code >> 16) & 0xFFFF);
    }

    if((error_code & 0xFFFF0000) != 0) {
        debug_debug("Accessed segment: 0x%04X\n", (error_code >> 16) & 0xFFFF);
    }
    auto pcb = ProcessManager::get_current_process();
    pcb->print();

    // 停止系统

    while(1) {
        asm("hlt");
    }
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