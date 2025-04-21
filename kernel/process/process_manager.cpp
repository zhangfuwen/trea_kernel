#include <kernel/kernel.h>

#include "kernel/elf_loader.h"
#include "kernel/process.h"
#include "kernel/syscall_user.h"
#include "kernel/vfs.h"
#include "lib/debug.h"

using kernel::FileAttribute;
using kernel::FileDescriptor;
using kernel::VFSManager;

void (*user_entry_point)();
void user_entry_wrapper()
{
    char* name = nullptr;
    int ret1 = syscall_fork();
    if(ret1 == 0) {
        log_err("child process\n");
        name = "child process";
    } else {
        name = "parent process";
    }
    // run user program
    while(1) {
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("hlt");
        log_debug("user_entry_wrapper called, %s\n", name);
    }
    log_debug("user_entry_wrapper called with entry_point: %x\n", user_entry_point);
    log_debug("user_entry_wrapper called with entry_point: %x\n", user_entry_point);
    log_debug("user_entry_wrapper called with entry_point: %x\n", user_entry_point);
    log_debug("user_entry_wrapper called with entry_point: %x\n", user_entry_point);
    log_debug("user_entry_wrapper called with entry_point: %x\n", user_entry_point);
    log_debug("user_entry_wrapper ended with entry_point: %x\n", user_entry_point);
    syscall_exit(0);
};

// 创建并执行一个新进程
// int32_t ProcessManager::execute_process(const char* path)
// {
//     // char * name = nullptr;
//     // int ret1 = syscall_fork();
//     // if (ret1 == 0) {
//     //     debug_err("child process\n");
//     //     name = "child process";
//     // }  else {
//     //     name = "parent process";
//     // }
//     // // run user program
//     // while (1) {
//     //     asm volatile("nop");
//     //     asm volatile("nop");
//     //     asm volatile("nop");
//     //     asm volatile("nop");
//     //     asm volatile("nop");
//     //     asm volatile("hlt");
//     //     debug_debug("user_entry_wrapper called, %s\n", name);
//     // }
//     // debug_debug("user_entry_wrapper called with entry_point: %x\n",
//     // user_entry_point); debug_debug("user_entry_wrapper called with entry_point:
//     // %x\n", user_entry_point); debug_debug("user_entry_wrapper called with
//     // entry_point: %x\n", user_entry_point); debug_debug("user_entry_wrapper
//     // called with entry_point: %x\n", user_entry_point);
//     // debug_debug("user_entry_wrapper called with entry_point: %x\n",
//     // user_entry_point); debug_debug("user_entry_wrapper ended with entry_point:
//     // %x\n", user_entry_point); syscall_exit(0);
//     //
//     //
//     // debug_debug("starting user_entry_point---: %x\n", user_entry_point);
//     // switch_to_user_mode((uint32_t)user_entry_wrapper);
//     //
//     // return 0;
// }