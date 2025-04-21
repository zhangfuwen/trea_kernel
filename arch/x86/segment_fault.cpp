#include "arch/x86/interrupt.h"
#include "lib/debug.h"

extern "C" void general_fault_errno_handler(uint32_t isr_no, uint32_t error_code)
{
    log_debug("fault occurred! NO. %d, Error code: %d\n", isr_no, error_code);
    auto pcb = ProcessManager::get_current_task();
    pcb->print();
}

// 新增通用保护故障处理函数
extern "C" void general_protection_fault_handler(uint32_t error_code)
{
    log_debug("General Protection Fault! Error code: %08x\n", error_code);

    // 解析错误代码位
    if(error_code & 0x1) {
        log_debug("  [Bit 0] External event (hardware interrupt)\n");
    } else {
        log_debug("  [Bit 0] Caused by program\n");
    }
    if(error_code & 0x2) {
        log_debug("  [Bit 1] Descriptor table: IDT\n");
    } else {
        log_debug("  [Bit 1] Descriptor table: GDT/LDT\n");
    }
    if(error_code & 0x4) {
        log_debug("  [Bit 2] LDT segment\n");
    } else {
        log_debug("  [Bit 2] Not LDT segment\n");
    }
    if(error_code & 0x8) {
        log_debug("  [Bit 3] Segment not present\n");
    }

    // 打印选择子相关信息（高16位）
    uint16_t selector = (error_code >> 16) & 0xFFFF;
    log_debug("  Selector: 0x%04X\n", selector);
    log_debug("  TI (Table Indicator): %s\n", (selector & 0x4) ? "LDT" : "GDT");
    log_debug("  RPL (Request Privilege Level): %d\n", selector & 0x3);
    log_debug("  Index: 0x%04X\n", selector >> 3);

    auto pcb = ProcessManager::get_current_task();
    if (pcb) pcb->print();

    // 停止系统

    while(1) {
        asm("hlt");
    }
}
extern "C" void stack_fault_handler(uint32_t error_code)
{
    log_debug("stack fault occurred! Error code: %d\n", error_code);
    // 根据错误码分析具体原因
    // 暂时直接停止系统
    while(1) {
        asm("hlt");
    }
}
extern "C" void segmentation_fault_handler(uint32_t error_code)
{
    log_debug("Segment fault occurred! Error code: %d\n", error_code);
    // 根据错误码分析具体原因
    if(error_code & 0x1) {
        log_debug("External event (not caused by program)\n");
    }
    if(error_code & 0x2) {
        log_debug("IDT gate type fault\n");
    }
    if(error_code & 0x4) {
        log_debug("LDT or IDT fault\n");
    }
    if(error_code & 0x8) {
        log_debug("Segment not present\n");
    }
    // 暂时直接停止系统
    while(1) {
        asm("hlt");
    }
}