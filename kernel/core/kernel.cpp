#include "kernel/kernel.h"
#include "lib/string.h"

#include <lib/serial.h>

#include "arch/x86/paging.h"
#include "lib/debug.h"

#define E_OK 0
#define E_NOT_COW 1
#define E_PANIC 2
int copyCOWPage(uint32_t fault_addr, uint32_t original_pgd, UserMemory& user_mm)
{
    auto& kernel_mm = Kernel::instance().kernel_mm();

    // 修改后的COW处理逻辑
    uint32_t flags = Kernel::instance().kernel_mm().paging().getPageFlags(fault_addr);

    // 检查COW标志
    if((flags & PAGE_COW) && (flags & PAGE_PRESENT)) {
        // 找到对应的物理页
        auto pd_index = (fault_addr >> 22) & 0x3FF;
        auto pde = ((uint32_t*)original_pgd)[pd_index];
        auto pt_phys = pde & 0xFFFFF000;
        auto pt_virt = pt_phys | 0xC0000000;
        auto pt_index = (fault_addr >> 12) & 0x3FF;
        auto pte = ((uint32_t*)pt_virt)[pt_index];
        uint32_t old_phys = pte & 0xFFFFF000;
        // 分配新物理页
        uint32_t new_phys = Kernel::instance().kernel_mm().alloc_pages(0, 0); // order=0表示分配单个页面
        if(!new_phys) {
            debug_err("COW failed to allocate new page\n");
            return E_PANIC;
        }

        // 旧的地址应该是可以读的，只是不可以写而已
        // 新物理页先映射到别处完成拷贝
        uint32_t tmp_virt = TEMP_MAPPING_VADDR; // 使用固定的临时映射地址
        Kernel::instance().kernel_mm().paging().mapPage(
            tmp_virt, new_phys, PAGE_PRESENT | PAGE_WRITE);
        memcpy((void*)tmp_virt, (void*)fault_addr, PAGE_SIZE);
        Kernel::instance().kernel_mm().paging().unmapPage(tmp_virt);

        // 更新页表项
        user_mm.map_pages(
            fault_addr & ~0xFFF, new_phys, PAGE_SIZE, (flags & ~PAGE_COW) | PAGE_WRITE);

        // 减少原页面的引用计数
        // 如果是复合页，BuddyAllocator会自动处理复合页的引用计数
        // 只会减少复合页首页的引用计数，不会导致错误释放
        //Kernel::instance().kernel_mm().decrement_ref_count(old_phys);
        return E_OK;
    }
    return E_NOT_COW;
}
// 缺页中断处理函数
void page_fault_handler(uint32_t error_code, uint32_t fault_addr)
{
    bool is_present = error_code & 0x1;      // 页面是否存在
    bool is_write = error_code & 0x2;        // 是否是写操作
    bool is_user = error_code & 0x4;         // 是否是用户态访问
    bool is_reserved = error_code & 0x8;     // 是否保留位被置位
    bool is_instruction = error_code & 0x10; // 是否是指令获取

    debug_debug("Page Fault at 0x%x, Error Code: 0x%x\n", fault_addr, error_code);
    debug_debug("Present: %d, Write: %d, User: %d, Reserved: %d, Instruction: %d\n", is_present,
        is_write, is_user, is_reserved, is_instruction);

    auto pcb = ProcessManager::get_current_task();
    pcb->print();
    auto pgd = pcb->context->user_mm.getPageDirectory();
    auto& user_mm = pcb->context->user_mm;

    // __printPDPTE((void*)fault_addr, (PageDirectory*)pgd);

    if(fault_addr >= 0x40000000 && fault_addr < 0xC0000000) {
        is_user = true;
    }

    // 检查是否是内核态还是用户态
    if(is_user) {
        // 用户态缺页中断
        if(!is_present) {
            // 页面不存在，需要分配新页面
            auto phys_page = Kernel::instance().kernel_mm().alloc_pages(0, 0); // order=0表示分配单个页面
            // debug_debug("Allocated pfn 0x%x\n", phys_page);
            if(phys_page) {
                // 建立用户态页表映射
                uint32_t flags = PAGE_USER | PAGE_WRITE | PAGE_PRESENT;
                if(!is_write) {
                    flags &= ~PAGE_WRITE; // 如果不是写操作，清除写权限
                }

                user_mm.map_pages(fault_addr & ~0xFFF, phys_page, PAGE_SIZE, flags);
                debug_debug("fixed page mapping\n");
                return;
            }
        } else if(is_write) {
            auto ret = copyCOWPage(fault_addr, pgd, user_mm);
            if (ret == E_OK) {
                debug_debug("copied page\n");
                return;
            } else if (ret == E_PANIC) {
                goto panic;
            }
        }
    } else {
        debug_debug("kernel page fault unexpected\n");
        // return;
        // // 内核态缺页中断
        // if(!is_present && !is_reserved) {
        //     // 分配内核页面
        //     auto pfn = Kernel::instance().kernel_mm().allocPage();
        //     uint32_t phys_page = pfn * PAGE_SIZE;
        //     if(phys_page) {
        //         // 建立内核态页表映射
        //         uint32_t flags = PAGE_WRITE | PAGE_PRESENT;
        //         Kernel::instance().kernel_mm().paging().mapPage(
        //             fault_addr & ~0xFFF, phys_page, flags);
        //         return;
        //     }
        // }
    }

panic:
    // 无法处理的缺页中断，输出错误信息
    debug_debug("Page Fault at 0x%x, Error Code: 0x%x\n", fault_addr, error_code);
    debug_debug("Present: %d, Write: %d, User: %d, Reserved: %d, Instruction: %d\n", is_present,
        is_write, is_user, is_reserved, is_instruction);
    printPDPTE((void*)fault_addr);
    debug_debug("will panic\n");
    asm volatile("hlt");

    // 终止当前进程或触发内核panic
    if(is_user) {
        // 终止用户进程
        // TODO: 实现进程终止逻辑
    } else {
        // 内核panic
        debug_debug("Kernel Panic: Unhandled Page Fault!\n");
        while(1)
            ;
    }
}

Kernel::Kernel() : memory_manager()
{
    debug_debug("Kernel::Kernel()");
}

void Kernel::init()
{
    serial_puts("kernel init\n");
    memory_manager.init();
    timer_ticks = 0;
}

bool Kernel::is_kernel_mode()
{
    uint16_t cs;
    asm volatile("mov %%cs, %0" : "=r"(cs));
    return (cs & 0x3) == 0; // 检查CPL是否为0
}

Kernel Kernel::instance0;
