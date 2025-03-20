#include "kernel/syscall.h"
#include "lib/console.h"
#include "lib/debug.h"

// 静态成员初始化
SyscallHandler SyscallManager::handlers[256];

// 初始化系统调用管理器
void SyscallManager::init() {
    // 初始化所有处理程序为默认处理程序
    for (int i = 0; i < 256; i++) {
        handlers[i] = reinterpret_cast<SyscallHandler>(defaultHandler);
    }
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
    debug_debug("syscall_num: %d\n", syscall_num);
    if (syscall_num < 256 && handlers[syscall_num]) {
        return handlers[syscall_num](arg1, arg2, arg3, arg4);
    }
    return -1;
}

// 默认系统调用处理程序
void SyscallManager::defaultHandler() {
    debug_debug("unhandled system call\n");
    Console::print("Unhandled system call\n");
}