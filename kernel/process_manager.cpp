#include "kernel/process.h"
#include "kernel/elf_loader.h"
#include "kernel/vfs.h"
#include "lib/debug.h"
#include "kernel/process.h"
#include "kernel/syscall_user.h"

using kernel::FileDescriptor;
using kernel::FileAttribute;
using kernel::VFSManager;


void (*user_entry_point)();
void user_entry_wrapper() {
    // run user program
    debug_debug("user_entry_wrapper called with entry_point: %x\n", user_entry_point);
    debug_debug("user_entry_wrapper called with entry_point: %x\n", user_entry_point);
    debug_debug("user_entry_wrapper called with entry_point: %x\n", user_entry_point);
    debug_debug("user_entry_wrapper called with entry_point: %x\n", user_entry_point);
    debug_debug("user_entry_wrapper called with entry_point: %x\n", user_entry_point);
    user_entry_point();
    debug_debug("user_entry_wrapper ended with entry_point: %x\n", user_entry_point);
    syscall_exit(0);

};

// 创建并执行一个新进程
int32_t ProcessManager::execute_process(const char* path) {
    // 打开可执行文件
    FileDescriptor* fd = VFSManager::instance().open(path);
    if (!fd) {
        debug_err("Failed to open file: %s\n", path);
        return -1;
    }

    // 读取文件属性
    auto attr = new FileAttribute();
    int ret = VFSManager::instance().stat(path, attr);
    if (ret < 0) {
        debug_err("Failed to get file stats: %s\n", path);
        delete attr;
        fd->close();
        return -1;
    }

    // 读取ELF文件内容
    uint8_t* elf_data = new uint8_t[attr->size];
    ssize_t size = fd->read(elf_data, attr->size);
    fd->close();

    if (size <= 0) {
        debug_err("Failed to read file: %s\n", path);
        delete[] elf_data;
        delete attr;
        return -1;
    }

    // 创建新进程
    uint32_t pid = ProcessManager::create_process(path);
    if (pid <= 0) {
        debug_err("Failed to create process: %s\n", path);
        delete[] elf_data;
        delete attr;
        return -1;
    }

    ProcessControlBlock* process = ProcessManager::get_current_process();
    ProcessManager::switch_process(pid);

    // 加载ELF文件
    if (!ElfLoader::load_elf(elf_data, size, 0x000000)) {
        debug_err("Failed to load ELF file: %s\n", path);
        delete[] elf_data;
        delete attr;
        return -1;
    }
    debug_debug("ELF file loaded successfully\n");

    const ElfHeader* header = (const ElfHeader*)(elf_data);
    
    // 分配内核栈
    uint8_t* stack = new uint8_t[4096];
    process->kernel_stack = (uint32_t)stack;
    process->esp0 = (uint32_t)stack + 4096 - 16; // 栈顶位置，预留一些空间

    // 设置进程入口点
    uint32_t entry_point = header->entry + 0x000000; // 加上基址偏移


    // 启动进程

    user_entry_point = (void (*)())entry_point;

    debug_debug("starting user_entry_point---: %x\n", user_entry_point);
    ElfLoader::switch_to_user_mode((uint32_t)user_entry_wrapper, (uint32_t)stack+1024);

    // 清理资源
    delete[] elf_data;
    delete attr;

    return pid;
}
