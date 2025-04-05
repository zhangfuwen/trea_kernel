#include <arch/x86/spinlock.h>
#include <arch/x86/atomic.h>
#include <arch/x86/interrupt.h>
#include <lib/debug.h>

// SpinLock类的实现已经移到头文件中
// 这里保留一些辅助函数

// 调试用锁状态检查
bool spin_is_locked(const SpinLock& lock) {
    return __atomic_load_n(&lock.locked, __ATOMIC_RELAXED);
}