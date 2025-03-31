#include "../include/arch/x86/paging.h"
#include "../include/lib/console.h"
#include <cstdint>

#include <lib/debug.h>
#include <lib/serial.h>

#include "kernel/kernel.h"

PageManager::PageManager() : nextFreePage(0x400000), curPgdVirt(nullptr) {}

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
    curPgdVirt =
        (PageDirectory*)(PAGE_DIRECTORY_ADDR + KERNEL_DIRECT_MAP_START); // 虚拟地址
    debug_debug("PAGE_DIRECTORY_ADDR: %x\n", PAGE_DIRECTORY_ADDR);
    debug_debug("KERNEL_DIRECT_MAP_START: %x\n", KERNEL_DIRECT_MAP_START);
    debug_debug("PageManager: currentPageDirectory: %x\n", curPgdVirt);
}

// 映射内核空间 896MB
void PageManager::mapKernelSpace()
{
    // 当前使用物理地址，实模式
    // 映射前4M
    auto* dir = reinterpret_cast<PageDirectory*>(PAGE_DIRECTORY_ADDR);
    auto* table1 = reinterpret_cast<PageTable*>(K_FIRST_4M_PT);
    dir->entries[0] = K_FIRST_4M_PT | 7; // Supervisor, read/write, present;
    for(int i = 0; i < 1024; i++) {
        table1->entries[i] = (i * 4096) | 7; // user can access, read/write, present
    }

    // map 0xC0000000
    uint32_t pteStart = 0xC0000000 >> 22;
    for(int j = 0; j < K_PAGE_TABLE_COUNT; j++) {
        auto* table = reinterpret_cast<PageTable*>(K_PAGE_TABLE_START + j * sizeof(PageTable));
        for(uint32_t i = 0; i < 1024; i++) {
            table->entries[i] = (i * 4096 + j * 1024 * 4096) | 3; // Supervisor, read/write, present
        }
        dir->entries[j + pteStart] =
            reinterpret_cast<uintptr_t>(table) | 3; // Supervisor, read/write, present
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
        debug_err("PageManager: currentPageDirectory is null\n");
        return;
    }

    // 检查页表是否存在
    PageTable* pt;
    if(!(curPgdVirt->entries[pd_index] & 0x1)) {
        // 创建新页表
        debug_debug("creating new page table\n");
        pt = (PageTable*)Kernel::instance().kernel_mm().allocPage();
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

    PageTable* pt =
        reinterpret_cast<PageTable*>(curPgdVirt->entries[pd_index] & 0xFFFFF000);
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

    PageTable* pt =
        reinterpret_cast<PageTable*>(curPgdVirt->entries[pd_index] & 0xFFFFF000);
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

    PageTable* pt =
        reinterpret_cast<PageTable*>(curPgdVirt->entries[pd_index] & 0xFFFFF000);
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

int PageManager::copyMemorySpaceCOW(PageDirectory* src, PageDirectory* dst)
{
    auto &kernel_mm = Kernel::instance().kernel_mm();
    // 复制页表项并设置COW标志
    for(uint32_t pde_idx = 0; pde_idx < 1024; pde_idx++) {
        // 复制所有页目录项（包含present和非present）
        dst->entries[pde_idx] = src->entries[pde_idx];

        // 仅处理存在的页表
        if(src->entries[pde_idx] & PAGE_PRESENT) {
            auto src_pt_paddr = src->entries[pde_idx] & 0xFFFFF000;
            auto dst_pt_paddr = Kernel::instance().kernel_mm().allocPage();
            PageTable* dst_pt = (PageTable*)kernel_mm.phys2Virt(dst_pt_paddr);
            PageTable* src_pt = (PageTable*)kernel_mm.phys2Virt(src_pt_paddr);

            // 复制所有页表项
            // debug_debug("copyMemorySpaceCOW: src_pt:%x, dst_pt:%x\n", src_pt, dst_pt);
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
            dst->entries[pde_idx] = dst_pt_paddr | (src->entries[pde_idx] & 0xFFF);
        }
    }
    return 0;
}