#include "process.h"
#include "buddy_allocator.h"
#include <kernel/vfs.h>
#include <lib/console.h>
#include <kernel/console_device.h>

#include "process.h"
#include "buddy_allocator.h"
#include <cstdint>

// 创建新的页目录
uint32_t ProcessManager::create_page_directory() {
    // 分配一个页面用作页目录
    uint32_t page_dir = BuddyAllocator::allocate_pages(1);
    if (!page_dir) return 0;

    // 初始化页目录
    uint32_t* dir = (uint32_t*)(page_dir);
    for (int i = 0; i < 1024; i++) {
        dir[i] = 0x00000002; // Supervisor, read/write, not present
    }

    // 映射内核空间（前4MB）
    uint32_t kernel_page_table = BuddyAllocator::allocate_pages(1);
    if (!kernel_page_table) {
        BuddyAllocator::free_pages(page_dir, 1);
        return 0;
    }

    uint32_t* table = (uint32_t*)(kernel_page_table);
    for (uint32_t i = 0; i < 1024; i++) {
        table[i] = (i * 4096) | 3; // Supervisor, read/write, present
    }

    // 设置内核空间的页目录项
    dir[0] = kernel_page_table | 3; // Supervisor, read/write, present

    return page_dir;
}

// 复制页表
void ProcessManager::copy_page_tables(uint32_t src_cr3, uint32_t dst_cr3) {
    uint32_t* src_dir = reinterpret_cast<uint32_t*>(src_cr3);
    uint32_t* dst_dir = reinterpret_cast<uint32_t*>(dst_cr3);

    // 遍历页目录
    for (int i = 0; i < 1024; i++) {
        if (!(src_dir[i] & 1)) continue; // 跳过不存在的页表

        // 对于内核空间（第一个页目录项），直接共享页表
        if (i == 0) {
            dst_dir[i] = src_dir[i];
            continue;
        }

        // 为用户空间分配新的页表
        uint32_t src_table = src_dir[i] & 0xFFFFF000;
        uint32_t dst_table = BuddyAllocator::allocate_pages(1);
        if (!dst_table) continue;

        // 复制页表内容
        uint32_t* src_entries = reinterpret_cast<uint32_t*>(src_table);
        uint32_t* dst_entries = reinterpret_cast<uint32_t*>(dst_table);
        for (int j = 0; j < 1024; j++) {
            if (!(src_entries[j] & 1)) continue; // 跳过不存在的页

            // 分配新的物理页面
            uint32_t new_page = BuddyAllocator::allocate_pages(1);
            if (!new_page) continue;

            // 复制页面内容
            uint32_t src_page = src_entries[j] & 0xFFFFF000;
            uint8_t* src_content = reinterpret_cast<uint8_t*>(src_page);
            uint8_t* dst_content = reinterpret_cast<uint8_t*>(new_page);
            for (uint32_t k = 0; k < 4096; k++) {
                dst_content[k] = src_content[k];
            }

            // 设置页表项，保持相同的权限位
            dst_entries[j] = (new_page) | (src_entries[j] & 0xFFF);
        }

        // 设置页目录项，保持相同的权限位
        dst_dir[i] = dst_table | (src_dir[i] & 0xFFF);
    }
}

// 复制进程内存空间
bool ProcessManager::copy_memory_space(ProcessControlBlock& parent, ProcessControlBlock& child) {
    // 分配新的页目录
    child.cr3 = ProcessManager::create_page_directory();
    if (!child.cr3) return false;

    // 复制页表和内存内容
    ProcessManager::copy_page_tables(parent.cr3, child.cr3);
    return true;
}

void ProcessManager::init() {
    next_pid = 1;
    current_process = 0;
    Console::print("ProcessManager initialized\n");
}

kernel::ConsoleFS console_fs;
// 创建新进程
uint32_t ProcessManager::create_process(const char* name) {
    uint32_t pid = next_pid++;
    if (pid >= MAX_PROCESSES) {
        return 0; // 进程数达到上限
    }

    ProcessControlBlock& pcb = processes[pid];
    pcb.pid = pid;
    pcb.state = PROCESS_NEW;
    pcb.priority = 1;
    pcb.time_slice = 100;
    pcb.total_time = 0;

    // 分配用户态栈（4MB）
    pcb.user_stack = BuddyAllocator::allocate_pages(1024);
    if (!pcb.user_stack) {
        return 0;
    }
    pcb.esp = pcb.user_stack + 4 * 1024 * 1024 - 16;

    // 分配内核态栈（64KB）
    pcb.kernel_stack = BuddyAllocator::allocate_pages(16);
    if (!pcb.kernel_stack) {
        BuddyAllocator::free_pages(pcb.user_stack, 1024);
        return 0;
    }
    pcb.esp0 = pcb.kernel_stack + 64 * 1024 - 16;

    // 设置段选择子
    pcb.cs = 0x1B; // 用户代码段选择子（RPL=3）
    pcb.ds = 0x23; // 用户数据段选择子（RPL=3）
    pcb.ss = 0x23; // 用户栈段选择子（RPL=3）

    // 复制进程名称
    for (int i = 0; i < 31 && name[i]; i++) {
        pcb.name[i] = name[i];
    }
    pcb.name[31] = '\0';

    // 初始化标准输入输出流
    pcb.stdin = console_fs.open("/dev/console");
    pcb.stdout = console_fs.open("/dev/console");
    pcb.stderr = console_fs.open("/dev/console");

    return pid;
}
kernel::ConsoleFS ProcessManager::console_fs;

// fork系统调用实现
int ProcessManager::fork() {
    ProcessControlBlock* parent = &processes[current_process];
    uint32_t child_pid = create_process(parent->name);
    if (child_pid == 0) return -1;

    ProcessControlBlock& child = processes[child_pid];

    // 复制父进程的寄存器状态
    child.esp = parent->esp;
    child.ebp = parent->ebp;
    child.eip = parent->eip;

    // 复制内存空间
    if (!copy_memory_space(*parent, child)) {
        // 如果内存复制失败，清理并返回错误
        child.state = PROCESS_TERMINATED;
        return -1;
    }

    // 设置子进程状态为就绪
    child.state = PROCESS_READY;

    // 父进程返回子进程PID，子进程返回0
    return child_pid;
}

// 获取当前运行的进程
ProcessControlBlock* ProcessManager::get_current_process() {
    return &processes[current_process];
}

// 切换到下一个进程
void ProcessManager::schedule() {
    // 简单的轮转调度
    uint32_t next = (current_process + 1) % MAX_PROCESSES;
    while (next != current_process) {
        if (processes[next].state == PROCESS_READY) {
            current_process = next;
            processes[next].state = PROCESS_RUNNING;
            break;
        }
        next = (next + 1) % MAX_PROCESSES;
    }
}

// 静态成员初始化
ProcessControlBlock ProcessManager::processes[ProcessManager::MAX_PROCESSES];
uint32_t ProcessManager::next_pid = 1;
uint32_t ProcessManager::current_process = 0;