#ifndef ARCH_X86_SPINLOCK_H
#define ARCH_X86_SPINLOCK_H

#include <cstdint>

class SpinLock {
public:
    SpinLock() : locked(0) {}

    void acquire() {
        while(__atomic_test_and_set(&locked, __ATOMIC_ACQUIRE)) {
            asm volatile("pause");
        }
    }

    void release() {
        __atomic_clear(&locked, __ATOMIC_RELEASE);
    }

    // 保存中断状态并获取锁
    void acquire_irqsave(uint32_t& flags) {
        asm volatile("pushf; pop %0" : "=r"(flags));
        asm volatile("cli");
        acquire();
    }

    // 释放锁并恢复中断状态
    void release_irqrestore(uint32_t flags) {
        release();
        asm volatile("push %0; popf" : : "r"(flags));
    }

private:
    volatile uint8_t locked;
};

#endif // ARCH_X86_SPINLOCK_H