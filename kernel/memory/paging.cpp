#include "../include/arch/x86/paging.h"
#include "../include/lib/console.h"
#include <cstdint>

#include <lib/debug.h>
#include <lib/serial.h>
#include <lib/string.h>

#include "kernel/kernel.h"

PageManager::PageManager() : curPgdVirt(nullptr) {}

void PageManager::init()
{
    // 初始化页目录
    serial_puts("PageManager: enter init\n");
    PageDirectory* pageDirectory = reinterpret_cast<PageDirectory*>(PAGE_DIRECTORY_ADDR);
    for(int i = 0; i < 1024; i++) {
        pageDirectory->entries[i] = 0x00000002; // Supervisor, read/write, not present
    }
    serial_puts("PageManager: pageDirectory init\n");

    mapKernelSpace();
    serial_puts("PageManager: kernel space mapped\n");

    // 加载页目录
    loadPageDirectory(reinterpret_cast<uintptr_t>(PAGE_DIRECTORY_ADDR));
    serial_puts("PageManager: pageDirectory load\n");
    enablePaging();
    serial_puts("PageManager: enable paging\n");
    curPgdVirt = (PageDirectory*)(PAGE_DIRECTORY_ADDR + KERNEL_DIRECT_MAP_START); // 虚拟地址
    log_debug("PAGE_DIRECTORY_ADDR: %x\n", PAGE_DIRECTORY_ADDR);
    log_debug("KERNEL_DIRECT_MAP_START: %x\n", KERNEL_DIRECT_MAP_START);
    log_debug("PageManager: currentPageDirectory: %x\n", curPgdVirt);
}

// 映射内核空间 896MB
void PageManager::mapKernelSpace()
{
    // 当前使用物理地址，实模式
    // 映射前4M
    auto* dir = reinterpret_cast<PageDirectory*>(PAGE_DIRECTORY_ADDR);
    auto* table1 = reinterpret_cast<PageTable*>(K_FIRST_4M_PT);
    dir->entries[0] = K_FIRST_4M_PT | 7; // user, read/write, present;
    for(int i = 0; i < 1024; i++) {
        table1->entries[i] = (i * 4096) | 7; // user can access, read/write, present
    }

    // map 0xC0000000
    uint32_t pteStart = 0xC0000000 >> 22;
    for(int j = 0; j < K_PAGE_TABLE_COUNT; j++) { // each pde
        auto* table = reinterpret_cast<PageTable*>(K_PAGE_TABLE_START + j * sizeof(PageTable));
        for(uint32_t i = 0; i < 1024; i++) {
            table->entries[i] = (i * 4096 + j * 1024 * 4096) | 3; // Supervisor, read/write, present
        }
        dir->entries[j + pteStart] =
            reinterpret_cast<uintptr_t>(table) | 3; // Supervisor, read/write, present
    }

    // 映射APIC区域 (0xFEC00000 - 0xFEEFFFFF)
    constexpr uint32_t APIC_START = 0xFEC00000;
    constexpr uint32_t APIC_END = 0xFEEFFFFF;
    
    for (uint32_t addr = APIC_START; addr <= APIC_END; addr += 0x1000) {
        uint32_t pd_index = addr >> 22;
        uint32_t pt_index = (addr >> 12) & 0x3FF;
        
        // 确保页表存在
        if (!(dir->entries[pd_index] & 0x1)) {
            auto* new_table = reinterpret_cast<PageTable*>(K_PAGE_TABLE_START + pd_index * sizeof(PageTable));
            dir->entries[pd_index] = reinterpret_cast<uintptr_t>(new_table) | 3;
        }
        
        auto* table = reinterpret_cast<PageTable*>(dir->entries[pd_index] & 0xFFFFF000);
        table->entries[pt_index] = addr | 0x13; // Supervisor, read/write, present, cache disabled
    }
}

void PageManager::loadPageDirectory(uint32_t dir)
{
    asm volatile("mov %0, %%cr3" : : "r"(dir));
}

void PageManager::enablePaging()
{
    uint32_t cr0_val;
    // 获取当前 CR0 寄存器的值
    asm volatile("mov %%cr0, %0" : "=r"(cr0_val));
    cr0_val |= 0x80000000; // 启用分页
    // 将修改后的 CR0 值写回 CR0 寄存器
    asm volatile("mov %0, %%cr0" : : "r"(cr0_val));
}

// 映射虚拟地址到物理地址
void PageManager::mapPage(uint32_t virt_addr, uint32_t phys_addr, uint32_t flags)
{
    // debug_debug("trying to map virt:%x, phys:%x, flags:%x\n", virt_addr,
    // phys_addr, flags);
    uint32_t pd_index = virt_addr >> 22;
    uint32_t pt_index = (virt_addr >> 12) & 0x3FF;

    // 获取页目录项
    if(!curPgdVirt) {
        log_err("PageManager: currentPageDirectory is null\n");
        return;
    }

    // 检查页表是否存在
    PageTable* pt;
    if(!(curPgdVirt->entries[pd_index] & 0x1)) {
        // 创建新页表
        log_debug("creating new page table\n");
        pt = (PageTable*)Kernel::instance().kernel_mm().alloc_pages(0, 0); // order=0表示分配单个页面
        // debug_debug("created phys:%x\n", pt);
        curPgdVirt->entries[pd_index] =
            reinterpret_cast<uint32_t>(pt) | 3; // Supervisor, read/write, present
    } else {
        pt = reinterpret_cast<PageTable*>(curPgdVirt->entries[pd_index] & 0xFFFFF000);
        // debug_debug("page table exists, phys: %x\n", pt);
    }

    // 设置页表项
    pt = (PageTable*)Kernel::instance().kernel_mm().phys2Virt((uint32_t)pt);
    // debug_debug("page table virt: %x\n", pt);
    pt->entries[pt_index] = (phys_addr & 0xFFFFF000) | (flags & 0xFFF) | 0x1; // Present
    // debug_debug("mapped virt:%x, phys:%x, flags:%x, pte:%x, pd_index:%x,
    // pt_index:%x\n", virt_addr, phys_addr, flags, pt->entries[pt_index],
    // pd_index, pt_index);
}

// 解除虚拟地址映射
void PageManager::unmapPage(uint32_t virt_addr)
{
    uint32_t pd_index = virt_addr >> 22;
    uint32_t pt_index = (virt_addr >> 12) & 0x3FF;

    if(!curPgdVirt || !(curPgdVirt->entries[pd_index] & 0x1))
        return;

    PageTable* pt = reinterpret_cast<PageTable*>(curPgdVirt->entries[pd_index] & 0xFFFFF000);
    pt->entries[pt_index] = 0x00000002; // Supervisor, read/write, not present
}

// 切换页目录
void PageManager::switchPageDirectory(PageDirectory* dirVirt, void* dirPhys)
{
    curPgdVirt = dirVirt;
    loadPageDirectory(reinterpret_cast<uint32_t>(dirPhys));
}

// 获取页表项标志位
uint32_t PageManager::getPageFlags(uint32_t virt_addr)
{
    uint32_t pd_index = virt_addr >> 22;
    uint32_t pt_index = (virt_addr >> 12) & 0x3FF;

    if(!curPgdVirt || !(curPgdVirt->entries[pd_index] & 0x1)) {
        return 0; // 页目录项不存在
    }

    PageTable* pt = reinterpret_cast<PageTable*>(curPgdVirt->entries[pd_index] & 0xFFFFF000);
    return pt->entries[pt_index] & 0xFFF; // 返回标志位
}

// 设置页表项标志位
void PageManager::setPageFlags(uint32_t virt_addr, uint32_t flags)
{
    uint32_t pd_index = virt_addr >> 22;
    uint32_t pt_index = (virt_addr >> 12) & 0x3FF;

    if(!curPgdVirt || !(curPgdVirt->entries[pd_index] & 0x1)) {
        return; // 页目录项不存在
    }

    PageTable* pt = reinterpret_cast<PageTable*>(curPgdVirt->entries[pd_index] & 0xFFFFF000);
    uint32_t entry = pt->entries[pt_index];
    if(entry & 0x1) { // 如果页面存在
        pt->entries[pt_index] = (entry & 0xFFFFF000) | (flags & 0xFFF);
    }
}

void PageManager::disablePaging()
{
    uint32_t cr0_val;
    // 获取当前 CR0 寄存器的值
    asm volatile("mov %%cr0, %0" : "=r"(cr0_val));
    cr0_val &= ~0x80000000; // 禁用分页
    // 将修改后的 CR0 值写回 CR0 寄存器
    asm volatile("mov %0, %%cr0" : : "r"(cr0_val));
}


int PageManager::copyMemorySpaceCOW(PageDirectory* src, PageDirectory* dstPgd)
{
    auto& kernel_mm = Kernel::instance().kernel_mm();
    for(int i = 0; i < 1024; i++) {
        dstPgd->entries[i] = 0x00000000; // Supervisor, read, not present
    }
    // 前4M空间
    dstPgd->entries[0] = src->entries[0];

    // 映射0xC0000000后896MB空间, 页表是已经存在的
    uint32_t kernelPteStart = 0xC0000000 >> 22;
    for(int j = kernelPteStart; j < kernelPteStart + K_PAGE_TABLE_COUNT; j++) {
        dstPgd->entries[j] = src->entries[j];
    }

    // 映射APIC区域 (0xFEC00000 - 0xFEEFFFFF)
    constexpr uint32_t APIC_START = 0xFEC00000;
    constexpr uint32_t APIC_END = 0xFEEFFFFF;
    uint32_t pd_index = APIC_START >> 22;
    log_debug("APIC_START pd_index:0x%x\n", pd_index);
    auto src_pt_paddr = src->entries[pd_index] & 0xFFFFF000;
    auto dst_pt_paddr = Kernel::instance().kernel_mm().alloc_pages(0, 0); // order=0表示分配单个页面
    log_debug("src_pt_paddr:0x%x, dst_pt_paddr:0x%x\n", src_pt_paddr, dst_pt_paddr);
    PageTable* dst_pt = (PageTable*)kernel_mm.phys2Virt(dst_pt_paddr);
    PageTable* src_pt = (PageTable*)kernel_mm.phys2Virt(src_pt_paddr);
    memcpy(dst_pt, src_pt, 4096);
    dstPgd->entries[pd_index] = dst_pt_paddr | 0x3; // Supervisor, read/write, present, cache disabled


    // 复制用户空间页表项并设置COW标志
    uint32_t userPteStart = USER_START >> 22;
    uint32_t userPteEnd = USER_END >> 22;
    for(uint32_t pde_idx = userPteStart; pde_idx < userPteEnd; pde_idx++) {
        // 复制所有页目录项（包含present和非present）
        dstPgd->entries[pde_idx] = src->entries[pde_idx];

        // 仅处理存在的页表
        if(src->entries[pde_idx] & PAGE_PRESENT) {
            auto src_pt_paddr = src->entries[pde_idx] & 0xFFFFF000;
            if (src_pt_paddr > 896 * 1024*1024) {
                log_err("PageManager: src_pt_paddr 0x%x, pde_idx:%d, pde:0x%x \n", src_pt_paddr, pde_idx, src->entries[pde_idx]);
                continue;
            }
            auto dst_pt_paddr = Kernel::instance().kernel_mm().alloc_pages(0, 0); // order=0表示分配单个页面
            log_debug("src_pt_paddr:0x%x, dst_pt_paddr:0x%x\n", src_pt_paddr, dst_pt_paddr);
            PageTable* dst_pt = (PageTable*)kernel_mm.phys2Virt(dst_pt_paddr);
            PageTable* src_pt = (PageTable*)kernel_mm.phys2Virt(src_pt_paddr);

            // 复制所有页表项
            log_debug("copyMemorySpaceCOW: src_pt:%x, dst_pt:%x\n", src_pt, dst_pt);
            for(uint32_t pte_idx = 0; pte_idx < 1024; pte_idx++) {
                // 如果是可写页面，设置COW标志
                if(src_pt->entries[pte_idx] & PAGE_WRITE) {
                    // 清除原页面的可写标志
                    src_pt->entries[pte_idx] &= ~PAGE_WRITE;
                    // 设置COW标志位（假设PAGE_COW是第9位）
                    src_pt->entries[pte_idx] |= PAGE_COW;

                    // 增加物理页引用计数
                    uint32_t phys_addr = src_pt->entries[pte_idx] & 0xFFFFF000;
                    Kernel::instance().kernel_mm().increment_ref_count(phys_addr);
                }
                // 复制修改后的条目到新页表
                dst_pt->entries[pte_idx] = src_pt->entries[pte_idx];
            }
            // 更新页目录项
            dstPgd->entries[pde_idx] = dst_pt_paddr | (src->entries[pde_idx] & 0xFFF);
        }
    }

    return 0;
}

void PagingValidate(PageDirectory * pd)
{
    for (int i = 0; i < 1024; i++) {
        if (pd->entries[i] & 0x1) {
            PADDR pt_paddr = pd->entries[i] & 0xFFFFF000;
            if(pt_paddr > 896*1024*1024) {
                log_err("PageManager: pt_paddr 0x%x, pd_index:%d, pde:0x%x \n", pt_paddr, i, pd->entries[i]);
                continue;
            }
            PageTable* pt = (PageTable*)Kernel::instance().kernel_mm().phys2Virt(pt_paddr);
            uint32_t pt_virt = (uint32_t)pt;
            if(pt_virt < USER_START && pt_virt > 0x500000) {
                log_err("PageManager: pt_paddr 0x%x, pd_index:%d, pde:0x%x \n", pt_paddr, i, pd->entries[i]);
                continue;
            }
            for (int j = 0; j < 1024; j++) {
                if (pt->entries[j] & 0x1) {
                    uint32_t phys_addr = pt->entries[j] & 0xFFFFF000;
                    uint32_t virt_addr = (i << 22) | (j << 12);
                    if (phys_addr > 896 * 1024*1024 && phys_addr != virt_addr) {
                        log_err("PagingValidte error: virt_addr: 0x%x, phys_addr: 0x%x\n", virt_addr,
                        phys_addr);
                    }

                }
            }
        }
    }
}
void printPTEFlags(uint32_t pte)
{
    // constexpr uint32_t PAGE_PRESENT = 0x1;        // 页面存在 (位0)
    // constexpr uint32_t PAGE_WRITE = 0x2;          // 可写 (位1)
    // constexpr uint32_t PAGE_USER = 0x4;           // 用户级 (位2)
    // constexpr uint32_t PAGE_WRITE_THROUGH = 0x8;  // 写透 (位3)
    // constexpr uint32_t PAGE_CACHE_DISABLE = 0x10; // 禁用缓存 (位4)
    // constexpr uint32_t PAGE_ACCESSED = 0x20;      // 已访问 (位5)
    // constexpr uint32_t PAGE_DIRTY = 0x40;         // 已修改 (位6)
    // constexpr uint32_t PAGE_GLOBAL = 0x100;       // 全局 (位8)
    // constexpr uint32_t PAGE_COW = 0x200;          // 写时复制 (位9), 系统自定义位
    bool present = pte & PAGE_PRESENT;
    bool write = pte & PAGE_WRITE;
    bool user = pte & PAGE_USER;
    bool writeThrough = pte & PAGE_WRITE_THROUGH;
    bool cacheDisable = pte & PAGE_CACHE_DISABLE;
    bool accessed = pte & PAGE_ACCESSED;
    bool dirty = pte & PAGE_DIRTY;
    bool global = pte & PAGE_GLOBAL;
    bool cow = pte & PAGE_COW;

    log_debug("present : %d\n", present);
    log_debug("write : %d\n", write);
    log_debug("user : %d\n", user);
    log_debug("writeThrough : %d\n", writeThrough);
    log_debug("cache disable : %d\n", cacheDisable);
    log_debug("accessed : %d\n", accessed);
    log_debug("dirty : %d\n", dirty);
    log_debug("global : %d\n", global);
    log_debug("cow : %d\n", cow);
}
void __printPDPTE(VADDR vaddr, PageDirectory* pdVirt)
{
    log_debug("vaddr : 0x%x\n", vaddr);
    auto pdPhys = Kernel::instance().kernel_mm().virt2Phys(pdVirt);

    auto fault_addr = (uint32_t)vaddr;
    auto pd_index = (fault_addr >> 22) & 0x3FF;
    auto pde = pdVirt->entries[fault_addr >> 22];
    auto pt_phys = pde & 0xFFFFF000;
    auto pt_virt = (PageTable*)Kernel::instance().kernel_mm().phys2Virt(pt_phys);
    auto pt_index = (fault_addr >> 12) & 0x3FF;
    auto pte = pt_virt->entries[pt_index];
    auto phys = pte & 0xFFFFF000;
    log_debug("PD: 0x%x(phys:0x%x), PD index:%d(0x%x), PDE:0x%x\n", pdVirt, pdPhys, pd_index,
        pd_index, pde);
    log_debug("PT: 0x%x(phys:0x%x), PT index:%d(0x%x), PTE:0x%x, phys:0x%x\n", pt_virt, pt_phys,
        pt_index, pt_index, pte, phys);
    printPTEFlags(pte);
}

void printPD(PageDirectory* pdVirt, uint32_t startIndex, uint32_t count)
{
    for(uint32_t i = startIndex; i < startIndex + count; i++) {
        auto pde = pdVirt->entries[i];
        log_debug("PDE: 0x%x\n", pde);
    }
}

void printPDE(PageDirectory* pdVirt, uint32_t index)
{
    auto pde = pdVirt->entries[index];
    log_debug("PDE: 0x%x\n", pde);
    log_debug("  Present:      %d\n",  pde & 0x1);
    log_debug("  RW:           %d\n", (pde >> 1) & 0x1);
    log_debug("  User/Super:   %d\n", (pde >> 2) & 0x1);
    log_debug("  PWT:          %d\n", (pde >> 3) & 0x1);
    log_debug("  PCD:          %d\n", (pde >> 4) & 0x1);
    log_debug("  Accessed:     %d\n", (pde >> 5) & 0x1);
    log_debug("  Dirty:        %d\n", (pde >> 6) & 0x1);
    log_debug("  Page Size:    %d\n", (pde >> 7) & 0x1);
    log_debug("  Global:       %d\n", (pde >> 8) & 0x1);
    log_debug("  Addr:      0x%x\n", (pde >> 12));
}

void printPDPTE(VADDR vaddr)
{

    auto& paging = Kernel::instance().kernel_mm().paging();
    auto pcb = ProcessManager::get_current_task();
    // auto pdVirt = paging.getCurrentPageDirectory();
    auto pdVirt = pcb->context->user_mm.getPageDirectory();
    __printPDPTE(vaddr, (PageDirectory*)pdVirt);
}