#include "elf_loader.h"
#include <kernel/vfs.h>
#include <arch/x86/gdt.h>
#include "process.h"
#include "scheduler.h"
#include "lib/debug.h"

using namespace kernel;
extern "C" void set_user_entry(uint32_t entry, uint32_t stack);

// 加载ELF文件
bool ElfLoader::load_elf(const void* elf_data, uint32_t size) {
    debug_debug("Loading elf...\n");
    if (!elf_data || size < sizeof(ElfHeader)) {
        debug_err("Invalid ELF file\n");
        return false;
    }

    const ElfHeader* header = static_cast<const ElfHeader*>(elf_data);

    // 检查ELF魔数
    if (header->magic != ELF_MAGIC) {
        debug_err("Invalid ELF magic number\n");
        return false;
    }

    // 检查文件类型
    if (header->type != ET_EXEC) {
        debug_err("Not an executable file\n");
        return false;
    }

    // 加载程序段
    const ProgramHeader* ph = static_cast<const ProgramHeader*>(
        static_cast<const void*>(static_cast<const char*>(elf_data) + header->phoff)
    );

    debug_debug("num segments: %d\n", header->phnum);
    for (uint16_t i = 0; i < header->phnum; i++) {
        if (ph[i].type == 1) { // LOAD类型段
            // 分配内存并复制段内容
            void* dest = reinterpret_cast<void*>(ph[i].vaddr);
            dest += 0x100000;
            const void* src = static_cast<const void*>(
                static_cast<const char*>(elf_data) + ph[i].offset
            );
            debug_debug("Copying segment %x to %x\n", dest, src);
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
    debug_debug("Done!\n");

    return true;
}

// 执行加载的程序
void ElfLoader::execute(uint32_t entry_point, ProcessControlBlock* process) {
    // 设置TSS的内核栈
    Scheduler::tss.esp0 = process->esp0;
    Scheduler::tss.ss0 = 0x10; // 内核数据段

    entry_point += 0x100000;
    char * data = (char*)entry_point;
    for(int i = 0; i< 20;i++) {
        debug_debug("entry_point: %x\n", data[i]);
    }

    asm volatile(
        "ljmp %0\n"
        : "=m" (entry_point)
    );
    
    // 设置用户程序入口点和栈指针
//    set_user_entry(entry_point, process->esp);
    
    // 通过调用门切换到用户态
    // 调用门选择子：0x33 (索引6，TI=0，RPL=3)
    asm volatile(
        "lcall $0x33, $0\n"  // 远调用到调用门
        :
        :
        : "memory"
    );
    
    // 注意：这里的代码不会被执行，因为调用门会切换到用户态
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