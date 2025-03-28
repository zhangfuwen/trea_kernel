#include "kernel/syscall.h"

#include <kernel/elf_loader.h>

#include "lib/console.h"
#include "lib/debug.h"
#include "kernel/syscall.h"

#include <arch/x86/paging.h>
#include <kernel/syscall_user.h>

#include "kernel/process.h"
#include "kernel/elf_loader.h"
#include "kernel/user_memory.h"
#include "lib/debug.h"
#include "kernel/vfs.h"


// 系统调用处理函数声明
int execveHandler(uint32_t path_ptr, uint32_t argv_ptr, uint32_t envp_ptr, uint32_t) {
    sys_execve(path_ptr, argv_ptr, envp_ptr);
}

// 静态成员初始化
SyscallHandler SyscallManager::handlers[256];

// 初始化系统调用管理器
void SyscallManager::init() {
    // 初始化所有处理程序为默认处理程序
    for (int i = 0; i < 256; i++) {
        handlers[i] = reinterpret_cast<SyscallHandler>(defaultHandler);
    }

    // 注册系统调用处理函数
    registerHandler(SYS_EXEC, execveHandler);
    registerHandler(SYS_EXIT, exitHandler);

    Console::print("SyscallManager initialized\n");
}

// 注册系统调用处理程序
void SyscallManager::registerHandler(uint32_t syscall_num, SyscallHandler handler) {
    if (syscall_num < 256) {
        handlers[syscall_num] = handler;
    }
}

// 系统调用处理函数
int SyscallManager::handleSyscall(uint32_t syscall_num, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4) {
    debug_debug("syscall_num: %d, arg1:%d, arg2:%d, arg3:%d, arg4:%d\n", syscall_num, arg1, arg2, arg3, arg4);
    if (syscall_num < 256 && handlers[syscall_num]) {
        auto ret = handlers[syscall_num](arg1, arg2, arg3, arg4);
        debug_debug("syscall_num: %d, ret:%d\n", syscall_num, ret);
        return ret;
    }
    debug_debug("syscall end ret -1 : num: %d, arg1:%d, arg2:%d, arg3:%d, arg4:%d\n");
    return -1;
}

// 默认系统调用处理程序
void SyscallManager::defaultHandler() {
    debug_debug("unhandled system call\n");
    Console::print("Unhandled system call\n");
}

// execve系统调用处理函数
int sys_execve(uint32_t path_ptr, uint32_t argv_ptr, uint32_t envp_ptr) {
    debug_debug("execve: path=%x, argv=%x, envp=%x\n", path_ptr, argv_ptr, envp_ptr);

    // 获取当前进程
    ProcessControlBlock* current = ProcessManager::get_current_process();
    if (!current) {
        return -1;
    }

    // 打开可执行文件
    const char* path = reinterpret_cast<const char*>(path_ptr);
    int fd = kernel::sys_open((uint32_t)path);
    if (fd < 0) {
        debug_err("Failed to open executable file: %s\n", path);
        return -1;
    }
    auto attr = new kernel::FileAttribute();
    int ret = kernel::VFSManager::instance().stat("/init", attr);
    if (ret < 0) {
        debug_err("Failed to stat /init!\n");
        return -1;
    }
    debug_debug("File stat ret %d, size %d!\n",ret, attr->size);

    // 读取文件内容
    auto filep = current->mm.allocate_area(attr->size, PAGE_WRITE, 0);
    // debug_debug("File allocated at %x\n", filep);
    int size = kernel::sys_read(fd, (uint32_t)filep, attr->size);
    if (size <= 0) {
        current->mm.free_area((uint32_t)filep);
        debug_err("Failed to read executable file\n");
        return -1;
    }
    debug_debug("File read size %d\n", size);
    kernel::sys_close(fd);


    // // 清理当前进程的地址空间
    // current->mm.unmap_pages(0x40000000, 0x80000000 - 0x40000000);

    // 加载ELF文件
    current->mm.map_pages(0x100000, 0x100000, 0x100000, PAGE_WRITE|PAGE_USER);
    if (!ElfLoader::load_elf(filep, size, 0)) {
        current->mm.free_area((uint32_t)filep);
        debug_err("Failed to load ELF file\n");
        return -1;
    }

    const ElfHeader* header = static_cast<const ElfHeader*>(filep);
    uint32_t entry_point = header->entry;
    debug_debug("entry_point: %x\n", entry_point);


    // 设置用户栈
    uint32_t stack_size = 0x100000; // 1MB栈空间
    uint32_t stack_top = 0xC0000000 - stack_size;
    void* stack = current->mm.allocate_area(stack_size, PAGE_WRITE, 0); // 用户栈区域
    debug_debug("stack: %x\n", stack);
    if (!stack) {
        debug_err("Failed to allocate user stack\n");
        return -1;
    }
    uint32_t * p = (uint32_t*)stack;
    *p =0x12345678;

    // TODO: 复制参数和环境变量到用户栈

    current->user_stack = (uint32_t)(stack + stack_size - 8192 + 2048);
    p = (uint32_t*)current->user_stack;
    *p = 0x12345678;


    ProcessManager::switch_to_user_mode(entry_point);
    current->mm.free_area((uint32_t)filep);


    return 0;
}