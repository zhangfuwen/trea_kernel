#include "kernel/kernel.h"

#include <lib/serial.h>

#include "arch/x86/paging.h"
#include "lib/debug.h"

// 缺页中断处理函数
void page_fault_handler(uint32_t error_code, uint32_t fault_addr) {
    bool is_present = error_code & 0x1;       // 页面是否存在
    bool is_write = error_code & 0x2;         // 是否是写操作
    bool is_user = error_code & 0x4;          // 是否是用户态访问
    bool is_reserved = error_code & 0x8;      // 是否保留位被置位
    bool is_instruction = error_code & 0x10;   // 是否是指令获取
    debug_debug("Page Fault at 0x%x, Error Code: 0x%x\n", fault_addr, error_code);
    debug_debug("Present: %d, Write: %d, User: %d, Reserved: %d, Instruction: %d\n",
              is_present, is_write, is_user, is_reserved, is_instruction);
    auto cr3 = Kernel::instance().kernel_mm().paging().getCurrentPageDirectory();
    debug_debug("cr3: 0x%x\n", cr3);
    auto pde = cr3->entries[fault_addr >> 22];
    debug_debug("pde: 0x%x\n", pde);
    auto pt_phys = pde & 0xFFFFF000;
    auto pt = (PageTable*)Kernel::instance().kernel_mm().getVirtualAddress(pt_phys);
    auto pt_index = (fault_addr >> 12) & 0x3FF;
    debug_debug("pt_index: 0x%x\n", pt_index);
    auto pte = pt->entries[pt_index];
    debug_debug("pte: 0x%x\n", pte);

    if (fault_addr >= 0x40000000 && fault_addr < 0xC0000000) {
        is_user = true;
    }

    // 检查是否是内核态还是用户态
    if (is_user) {
        // 用户态缺页中断
        if (!is_present) {
            // 页面不存在，需要分配新页面
            auto phys_page = Kernel::instance().kernel_mm().allocPage();
            // debug_debug("Allocated pfn 0x%x\n", phys_page);
            if (phys_page) {
                // 建立用户态页表映射
                uint32_t flags = PAGE_USER | PAGE_WRITE | PAGE_PRESENT;
                if (!is_write) {
                    flags &= ~PAGE_WRITE;  // 如果不是写操作，清除写权限
                }
                Kernel::instance().kernel_mm().paging().mapPage(fault_addr & ~0xFFF, phys_page, flags);
                // debug_debug("fixed page mapping");
                return;
            }
        } else if (is_write && !is_reserved) {
            // 写保护异常，可能需要启用写权限
            uint32_t flags = Kernel::instance().kernel_mm().paging().getPageFlags(fault_addr);
            if (!(flags & PAGE_WRITE)) {
                // 更新页表项，添加写权限
                Kernel::instance().kernel_mm().paging().setPageFlags(fault_addr, flags | PAGE_WRITE);
                // debug_debug("fixed page mapping write permission");
                return;
            }
        }
    } else {
        debug_debug("kernel page fault unexpected");
        return;
        // 内核态缺页中断
        if (!is_present && !is_reserved) {
            // 分配内核页面
            auto pfn = Kernel::instance().kernel_mm().allocPage();
            uint32_t phys_page = pfn * PAGE_SIZE;
            if (phys_page) {
                // 建立内核态页表映射
                uint32_t flags = PAGE_WRITE | PAGE_PRESENT;
                Kernel::instance().kernel_mm().paging().mapPage(fault_addr & ~0xFFF, phys_page, flags);
                return;
            }
        }
    }

    // 无法处理的缺页中断，输出错误信息
    debug_debug("Page Fault at 0x%x, Error Code: 0x%x\n", fault_addr, error_code);
    debug_debug("Present: %d, Write: %d, User: %d, Reserved: %d, Instruction: %d\n",
              is_present, is_write, is_user, is_reserved, is_instruction);
    
    // 终止当前进程或触发内核panic
    if (is_user) {
        // 终止用户进程
        // TODO: 实现进程终止逻辑
    } else {
        // 内核panic
        debug_debug("Kernel Panic: Unhandled Page Fault!\n");
        while(1);
    }
}

Kernel::Kernel() : memory_manager() {
  debug_debug("Kernel::Kernel()");
}

void Kernel::init() {
    serial_puts("kernel init\n");
    memory_manager.init();
}

bool Kernel::is_kernel_mode() {
    uint16_t cs;
    asm volatile("mov %%cs, %0" : "=r"(cs));
    return (cs & 0x3) == 0;  // 检查CPL是否为0
}

Kernel Kernel::instance0;
