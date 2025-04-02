#include "kernel/syscall.h"

#include <kernel/elf_loader.h>

#include "kernel/syscall.h"
#include "lib/console.h"
#include "lib/debug.h"

#include <arch/x86/paging.h>
#include <kernel/kernel.h>
#include <kernel/syscall_user.h>

#include "kernel/elf_loader.h"
#include "kernel/process.h"
#include "kernel/user_memory.h"
#include "kernel/vfs.h"
#include "lib/debug.h"
#include "lib/time.h"

// 系统调用处理函数声明
int sys_mkdir(const char* path)
{
    return kernel::VFSManager::instance().mkdir(path);
}

int mkdirHandler(uint32_t path_ptr, uint32_t, uint32_t, uint32_t)
{
    const char* path = reinterpret_cast<const char*>(path_ptr);
    return sys_mkdir(path);
}

int sys_unlink(const char* path)
{
    return kernel::VFSManager::instance().unlink(path);
}

int unlinkHandler(uint32_t path_ptr, uint32_t, uint32_t, uint32_t)
{
    const char* path = reinterpret_cast<const char*>(path_ptr);
    return sys_unlink(path);
}

int sys_rmdir(const char* path)
{
    return kernel::VFSManager::instance().rmdir(path);
}

int rmdirHandler(uint32_t path_ptr, uint32_t, uint32_t, uint32_t)
{
    const char* path = reinterpret_cast<const char*>(path_ptr);
    return sys_rmdir(path);
}

int sys_stat(const char* path, kernel::FileAttribute* attr)
{
    return kernel::VFSManager::instance().stat(path, attr);
}

int statHandler(uint32_t path_ptr, uint32_t attr_ptr, uint32_t, uint32_t)
{
    auto pcb = ProcessManager::get_current_process();
    const char* path = reinterpret_cast<const char*>(path_ptr);
    kernel::FileAttribute* attr = reinterpret_cast<kernel::FileAttribute*>(attr_ptr);
    
    return sys_stat(path, attr);
}

int execveHandler(uint32_t path_ptr, uint32_t argv_ptr, uint32_t envp_ptr, uint32_t)
{
    auto ret = sys_execve(path_ptr, argv_ptr, envp_ptr, nullptr);
    return ret;
}

int nanosleepHandler(uint32_t req_ptr, uint32_t rem_ptr, uint32_t, uint32_t) {
    // 将用户空间指针转换为内核可访问的指针
    timespec* req = reinterpret_cast<timespec*>(req_ptr);

    // 计算总纳秒数（简单实现，不考虑溢出）
    uint64_t total_ns = req->tv_sec * 1000000000ULL + req->tv_nsec;

    // 转换为时钟滴答数（假设1 tick=10ms）
    uint32_t ticks = total_ns >> 24;

    // 挂起当前进程
    ProcessManager::sleep_current_process(ticks);

    return 0; // 返回成功
}


// 静态成员初始化
SyscallHandler SyscallManager::handlers[256];

// 初始化系统调用管理器
void SyscallManager::init()
{
    // 初始化所有处理程序为默认处理程序
    for(int i = 0; i < 256; i++) {
        handlers[i] = reinterpret_cast<SyscallHandler>(defaultHandler);
    }

    // 注册系统调用处理函数
    registerHandler(SYS_EXEC, execveHandler);
    registerHandler(SYS_EXIT, exitHandler);
    registerHandler(SYS_NANOSLEEP, nanosleepHandler);
    registerHandler(SYS_GETPID, getpid_handler);
    registerHandler(SYS_STAT, statHandler);
    registerHandler(SYS_MKDIR, mkdirHandler);
    registerHandler(SYS_UNLINK, unlinkHandler);
    registerHandler(SYS_RMDIR, rmdirHandler);

    Console::print("SyscallManager initialized\n");
}

int getpid_handler(uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{
    return sys_getpid();
}

int sys_getpid() {
    auto pcb =  ProcessManager::get_current_process();
    debug_info("pcb is %x, pid is %d\n", pcb, pcb->pid);
    return pcb->pid;
}

// 注册系统调用处理程序
void SyscallManager::registerHandler(uint32_t syscall_num, SyscallHandler handler)
{
    if(syscall_num < 256) {
        handlers[syscall_num] = handler;
    }
}

// 系统调用处理函数
int SyscallManager::handleSyscall(
    uint32_t syscall_num, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4)
{
    debug_debug("syscall_num: %d, arg1:%d, arg2:%d, arg3:%d, arg4:%d\n", syscall_num, arg1, arg2,
        arg3, arg4);
    if(syscall_num < 256 && handlers[syscall_num]) {
        auto ret = handlers[syscall_num](arg1, arg2, arg3, arg4);
        debug_debug("syscall_num: %d, ret:%d\n", syscall_num, ret);
        return ret;
    }
    debug_debug("syscall end ret -1 : num: %d, arg1:%d, arg2:%d, arg3:%d, arg4:%d\n");
    return -1;
}

// 默认系统调用处理程序
void SyscallManager::defaultHandler()
{
    debug_debug("unhandled system call\n");
    Console::print("Unhandled system call\n");
}

// execve系统调用处理函数
int sys_execve(uint32_t path_ptr, uint32_t argv_ptr, uint32_t envp_ptr, ProcessControlBlock* pcb)
{
    debug_debug("execve: path=%x, argv=%x, envp=%x\n", path_ptr, argv_ptr, envp_ptr);

    // 打开可执行文件
    const char* path = reinterpret_cast<const char*>(path_ptr);
    int fd = kernel::sys_open((uint32_t)path, pcb);
    if(fd < 0) {
        debug_err("Failed to open executable file: %s\n", path);
        return -1;
    }
    debug_debug("open successful, fd: %d\n", fd);
    auto attr = new kernel::FileAttribute();
    int ret = kernel::VFSManager::instance().stat("/init", attr);
    if(ret < 0) {
        debug_err("Failed to stat /init!\n");
        return -1;
    }
    debug_debug("File stat ret %d, size %d!\n", ret, attr->size);

    // 读取文件内容
    auto filep = pcb->user_mm.allocate_area(attr->size, PAGE_WRITE, 0);
    debug_debug("File allocated at %x\n", filep);
    auto pages = Kernel::instance().kernel_mm().alloc_pages(attr->size / 4096);
    for(int i = 0; i < attr->size / 4096; i++) {
        pcb->user_mm.map_pages((uint32_t)filep + i * 4096, pages[i].pfn * 4096, 4096,
            PAGE_USER | PAGE_WRITE | PAGE_PRESENT);
    }
    debug_debug("File allocated at %x\n", filep);
    int size = kernel::sys_read(fd, (uint32_t)filep, attr->size, pcb);
    if(size <= 0) {
        pcb->user_mm.free_area((uint32_t)filep);
        debug_err("Failed to read executable file\n");
        return -1;
    }
    debug_debug("File read size %d\n", size);
    kernel::sys_close(fd, pcb);

    // // 清理当前进程的地址空间
    // pcb->mm.unmap_pages(0x40000000, 0x80000000 - 0x40000000);

    // 加载ELF文件
    pcb->user_mm.map_pages(0x100000, 0x100000, 0x100000, PAGE_WRITE | PAGE_USER);

    // auto loadAddr = pcb->user_mm.allocate_area(0x4000, PAGE_WRITE, 0);
    // auto paddr = Kernel::instance().kernel_mm().allocPage();
    // pcb->user_mm.map_pages((uint32_t)loadAddr, paddr, 0x1000, PAGE_WRITE |
    // PAGE_USER|PAGE_PRESENT);
    auto loadAddr = 0;
    if(!ElfLoader::load_elf(filep, size, (uint32_t)loadAddr)) {
        pcb->user_mm.free_area((uint32_t)filep);
        debug_err("Failed to load ELF file\n");
        return -1;
    }
    pcb->user_mm.map_pages(0x100000, 0x100000, 0x100000, PAGE_USER | PAGE_WRITE | PAGE_PRESENT);

    const ElfHeader* header = static_cast<const ElfHeader*>(filep);
    uint32_t entry_point = header->entry;
    debug_debug("entry_point: %x\n", entry_point);

    ProcessManager::switch_to_user_mode(entry_point + (uint32_t)loadAddr, pcb);
    // pcb->mm.free_area((uint32_t)filep);

    return 0;
}
