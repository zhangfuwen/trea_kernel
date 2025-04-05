#ifndef KERNEL_MUTEX_H
#define KERNEL_MUTEX_H

#include <cstdint>
#include <arch/x86/spinlock.h>
#include <arch/x86/atomic.h>

// 互斥锁状态定义
#define MUTEX_UNLOCKED 0
#define MUTEX_LOCKED 1

// 互斥锁类型
#define MUTEX_NORMAL 0    // 非递归锁
#define MUTEX_RECURSIVE 1  // 递归锁

// 互斥锁超时定义
#define MUTEX_WAIT_FOREVER 0xFFFFFFFF

namespace kernel {

/**
 * 互斥锁(Mutex)类
 * 
 * 与自旋锁不同，互斥锁在无法获取时会让线程睡眠而不是忙等
 * 支持递归锁和非递归锁两种模式
 * 支持超时等待
 * 提供死锁检测功能
 */
class Mutex {
public:
    /**
     * 构造函数
     * @param type 锁类型，MUTEX_NORMAL或MUTEX_RECURSIVE
     */
    Mutex(uint8_t type = MUTEX_NORMAL);

    /**
     * 获取锁
     * @param timeout 超时时间(单位:时钟滴答)，MUTEX_WAIT_FOREVER表示无限等待
     * @return 是否成功获取锁
     */
    bool lock(uint32_t timeout = MUTEX_WAIT_FOREVER);

    /**
     * 尝试获取锁，如果锁已被占用则立即返回
     * @return 是否成功获取锁
     */
    bool tryLock();

    /**
     * 释放锁
     * @return 是否成功释放锁
     */
    bool unlock();

    /**
     * 检查锁是否被占用
     * @return 锁是否被占用
     */
    bool isLocked() const;

    /**
     * 获取锁的拥有者进程ID
     * @return 拥有者进程ID，如果锁未被占用则返回0
     */
    uint32_t getOwner() const;

private:
    volatile uint32_t state;       // 锁状态
    volatile uint32_t owner;        // 锁的拥有者(进程ID)
    volatile uint32_t recursion;    // 递归计数
    uint8_t type;                   // 锁类型
    SpinLock spin_lock;             // 内部自旋锁，用于保护互斥锁的状态

    // 等待队列相关
    struct WaitNode {
        uint32_t pid;               // 等待的进程ID
        WaitNode* next;             // 下一个等待节点
    };

    WaitNode* wait_list;            // 等待队列头

    // 唤醒等待队列中的下一个进程
    void wakeNextWaiter();

    // 将当前进程加入等待队列
    void addWaiter();

    // 从等待队列中移除指定进程
    void removeWaiter(uint32_t pid);

    // 检查是否会导致死锁
    bool wouldDeadlock() const;
};

} // namespace kernel

#endif // KERNEL_MUTEX_H