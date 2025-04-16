#include "kernel/process.h"
#include "kernel/scheduler.h"
#include "kernel/syscall.h"
#include "lib/debug.h"

// exit系统调用处理函数
int exitHandler(uint32_t status, uint32_t, uint32_t, uint32_t)
{
    debug_debug("Process exiting with status %d\n", status);

    // 获取当前进程
    Task* current = ProcessManager::get_current_task();
    if(!current) {
        return -1;
    }

    // 设置进程状态为已退出
    current->state = ProcessState::EXITED;
    current->exit_status = status;

    // 释放进程资源
    // TODO: 实现资源清理逻辑

    // 切换到其他进程
    Scheduler::schedule();

    return 0;
}