#include <cstdint>
#include "lib/console.h"
#include "arch/x86/paging.h"

struct PageDirectory;
// 声明外部汇编函数
extern "C" uint32_t cr0_asm();
extern "C" uint32_t cr3_asm();

void PageManager::init() {
    // 初始化页目录
    Console::print("PageManager: enter init\n");
    PageDirectory* pageDirectory = reinterpret_cast<PageDirectory*>(allocPages(1));
    for (int i = 0; i < 1024; i++) {
        pageDirectory->entries[i] = 0x00000002; // Supervisor, read/write, not present
    }
    Console::print("PageManager: pageDirectory init\n");

    // 映射前40MB的内存（内核空间）
    mapKernelSpace(pageDirectory, 10);
    Console::print("PageManager: pageDirectory map\n");

    // 加载页目录
    loadPageDirectory(reinterpret_cast<uintptr_t>(pageDirectory));
    Console::print("PageManager: pageDirectory load\n");
    enablePaging();
    Console::print("PageManager: enable paging\n");
}

uint32_t PageManager::allocPages(int count) {
    uint32_t page = PageManager::nextFreePage;
    PageManager::nextFreePage += 4096*count;
    return page;
}


uint32_t PageManager::allocPage() {
    uint32_t page = PageManager::nextFreePage;
    PageManager::nextFreePage += 4096;
    return page;
}

void PageManager::mapKernelSpace(PageDirectory* dir, int count) {

    for(int j = 0; j < count; j++) {
        PageTable* table = reinterpret_cast<PageTable*>(allocPage());
        // 直接映射前4MB物理内存
        for (uint32_t i = 0; i < 1024; i++) {
            table->entries[i] = (i * 4096 + j * 1024*4096) | 3; // Supervisor, read/write, present
        }

        // 设置页目录项
        dir->entries[j] = reinterpret_cast<uintptr_t>(table) | 3; // Supervisor, read/write, present
    }
}


// 加载页目录到 CR3 寄存器
void PageManager::loadPageDirectory(uint32_t dir) {
    asm volatile("mov %0, %%cr3" : : "r"(dir));
}

// 启用分页机制
void PageManager::enablePaging() {
    uint32_t cr0_val;
    // 获取当前 CR0 寄存器的值
    asm volatile("mov %%cr0, %0" : "=r"(cr0_val));
    cr0_val |= 0x80000000; // 启用分页
    // 将修改后的 CR0 值写回 CR0 寄存器
    asm volatile("mov %0, %%cr0" : : "r"(cr0_val));
}
// static void loadPageDirectory(uint32_t dir) {
//     asm volatile("mov cr3_asm, %0" : : "r"(dir));
// }

// static void enablePaging() {
//     uint32_t cr0_val = cr0_asm();
//     cr0_val |= 0x80000000; // 启用分页
//     asm volatile("mov cr0_asm, %0" : : "r"(cr0_val));
// }

// 静态成员初始化
uint32_t PageManager::nextFreePage = 0x400000; // 从4MB开始分配页