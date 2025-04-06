#ifndef ARCH_X86_SPINLOCK_H
#define ARCH_X86_SPINLOCK_H

#include <cstdint>

class SpinLock {
public:
    SpinLock() : locked(0) {}
    
    // 拷贝构造函数
    SpinLock([[maybe_unused]] const SpinLock& other) : locked(0) {}
    
    // 赋值运算符
    SpinLock& operator=(const SpinLock& other) {
        // 自赋值检查
        if (this != &other) {
            // 确保锁是释放状态
            locked = 0;
        }
        return *this;
    }

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

// 全局函数封装
inline void spin_lock(SpinLock* lock) {
    if (lock) lock->acquire();
}

inline void spin_unlock(SpinLock* lock) {
    if (lock) lock->release();
}

#endif // ARCH_X86_SPINLOCK_H