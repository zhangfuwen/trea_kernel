#include "elf_loader.h"
#include <kernel/vfs.h>
#include <arch/x86/gdt.h>
#include "process.h"
#include "scheduler.h"

using namespace kernel;

// 加载ELF文件
bool ElfLoader::load_elf(const void* elf_data, uint32_t size) {
    if (!elf_data || size < sizeof(ElfHeader)) {
        Console::print("Invalid ELF file\n");
        return false;
    }

    const ElfHeader* header = static_cast<const ElfHeader*>(elf_data);

    // 检查ELF魔数
    if (header->magic != ELF_MAGIC) {
        Console::print("Invalid ELF magic number\n");
        return false;
    }

    // 检查文件类型
    if (header->type != ET_EXEC) {
        Console::print("Not an executable file\n");
        return false;
    }

    // 加载程序段
    const ProgramHeader* ph = static_cast<const ProgramHeader*>(
        static_cast<const void*>(static_cast<const char*>(elf_data) + header->phoff)
    );

    for (uint16_t i = 0; i < header->phnum; i++) {
        if (ph[i].type == 1) { // LOAD类型段
            // 分配内存并复制段内容
            void* dest = reinterpret_cast<void*>(ph[i].vaddr);
            const void* src = static_cast<const void*>(
                static_cast<const char*>(elf_data) + ph[i].offset
            );
            // 复制段内容到目标地址
            for (uint32_t j = 0; j < ph[i].filesz; j++) {
                static_cast<char*>(dest)[j] = static_cast<const char*>(src)[j];
            }
            // 清零剩余内存
            for (uint32_t j = ph[i].filesz; j < ph[i].memsz; j++) {
                static_cast<char*>(dest)[j] = 0;
            }
        }
    }

    return true;
}

// 执行加载的程序
void ElfLoader::execute(uint32_t entry_point, ProcessControlBlock* process) {
    // 设置TSS的内核栈
    Scheduler::tss.esp0 = process->esp0;
    Scheduler::tss.ss0 = 0x10; // 内核数据段

    // 切换到用户态并执行程序
    asm volatile(
        "movw %0, %%ax\n"   // 设置数据段
        "movw %%ax, %%ds\n"
        "movw %%ax, %%es\n"
        "movw %%ax, %%fs\n"
        "movw %%ax, %%gs\n"
        "pushl %1\n"        // SS
        "pushl %2\n"        // ESP
        "pushfl\n"          // EFLAGS
        "popl %%eax\n"      // 获取EFLAGS
        "orl $0x200, %%eax\n" // 设置IF位（允许中断）
        "pushl %%eax\n"     // 修改后的EFLAGS
        "pushl %3\n"        // CS
        "pushl %4\n"        // EIP
        "iret\n"            // 切换到用户态
        :
        : "rm"(process->ds), "rm"(process->ss), "rm"(process->esp),
            "rm"(process->cs), "rm"(entry_point)
        : "eax"
    );
}

// exec系统调用实现
int ElfLoader::exec(const char* path, char* const argv[]) {
    // 通过VFS打开文件
    kernel::FileDescriptor* fd = kernel::VFSManager::instance().open(path);
    if (!fd) {
        return -1;
    }

    // 获取文件属性
    kernel::FileAttribute attr;
    if (kernel::VFSManager::instance().stat(path, &attr) < 0) {
        fd->close();
        return -1;
    }

    // 读取文件内容
    void* elf_data = new uint8_t[attr.size];
    if (fd->read(elf_data, attr.size) != attr.size) {
        delete[] static_cast<uint8_t*>(elf_data);
        fd->close();
        return -1;
    }
    fd->close();

    // 加载ELF文件
    if (!load_elf(elf_data, attr.size)) {
        delete[] static_cast<uint8_t*>(elf_data);
        return -1;
    }

    // 获取ELF头和入口点地址
    const ElfHeader* header = static_cast<const ElfHeader*>(elf_data);
    uint32_t entry_point = header->entry;

    // 释放原始文件数据
    delete[] static_cast<uint8_t*>(elf_data);
    elf_data = nullptr; // 防止悬空指针

    // 获取当前进程
    ProcessControlBlock* current = ProcessManager::get_current_process();

    // 执行新程序
    execute(entry_point, current);
    return 0;
}