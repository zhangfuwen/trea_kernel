#include <kernel/mutex.h>
#include <kernel/process.h>
#include <lib/debug.h>
#include <arch/x86/interrupt.h>

namespace kernel {

Mutex::Mutex(uint8_t type) : state(MUTEX_UNLOCKED), owner(0), recursion(0), type(type), wait_list(nullptr) {
    // 初始化互斥锁
}

bool Mutex::lock(uint32_t timeout) {
    // 获取当前进程
    auto current = ProcessManager::get_current_task();
    if (!current) {
        debug_err("Mutex::lock: No current process\n");
        return false;
    }

    uint32_t pid = current->task_id;
    uint32_t elapsed = 0;

    // 禁用中断并获取自旋锁，保护互斥锁状态
    uint32_t flags;
    spin_lock.acquire_irqsave(flags);

    // 检查是否会导致死锁
    if (wouldDeadlock()) {
        debug_err("Mutex::lock: Deadlock detected for pid %d\n", pid);
        spin_lock.release_irqrestore(flags);
        return false;
    }

    // 如果是递归锁且当前进程已经持有锁，增加递归计数
    if (type == MUTEX_RECURSIVE && owner == pid) {
        recursion++;
        spin_lock.release_irqrestore(flags);
        return true;
    }

    // 尝试获取锁
    while (state == MUTEX_LOCKED) {
        // 如果是tryLock或已超时，则返回失败
        if (timeout == 0 || (timeout != MUTEX_WAIT_FOREVER && elapsed >= timeout)) {
            spin_lock.release_irqrestore(flags);
            return false;
        }

        // 将当前进程加入等待队列
        addWaiter();

        // 释放自旋锁并允许中断
        spin_lock.release_irqrestore(flags);

        // 让当前进程睡眠
        ProcessManager::sleep_current_process(1); // 睡眠1个时钟滴答
        elapsed++;

        // 重新获取自旋锁并禁用中断
        spin_lock.acquire_irqsave(flags);

        // 从等待队列中移除当前进程
        removeWaiter(pid);
    }

    // 获取锁
    state = MUTEX_LOCKED;
    owner = pid;
    recursion = 1;

    // 释放自旋锁并恢复中断
    spin_lock.release_irqrestore(flags);
    return true;
}

bool Mutex::tryLock() {
    return lock(0);
}

bool Mutex::unlock() {
    // 获取当前进程
    auto current = ProcessManager::get_current_task();
    if (!current) {
        debug_err("Mutex::unlock: No current process\n");
        return false;
    }

    uint32_t pid = current->task_id;

    // 禁用中断并获取自旋锁
    uint32_t flags;
    spin_lock.acquire_irqsave(flags);

    // 检查是否是锁的拥有者
    if (owner != pid) {
        debug_err("Mutex::unlock: Process %d is not the owner of this mutex\n", pid);
        spin_lock.release_irqrestore(flags);
        return false;
    }

    // 如果是递归锁，减少递归计数
    if (type == MUTEX_RECURSIVE && recursion > 1) {
        recursion--;
        spin_lock.release_irqrestore(flags);
        return true;
    }

    // 释放锁
    state = MUTEX_UNLOCKED;
    owner = 0;
    recursion = 0;

    // 唤醒等待队列中的下一个进程
    wakeNextWaiter();

    // 释放自旋锁并恢复中断
    spin_lock.release_irqrestore(flags);
    return true;
}

bool Mutex::isLocked() const {
    return state == MUTEX_LOCKED;
}

uint32_t Mutex::getOwner() const {
    return owner;
}

void Mutex::wakeNextWaiter() {
    // 如果等待队列为空，直接返回
    if (!wait_list) {
        return;
    }

    // 获取等待队列中的第一个进程
    WaitNode* waiter = wait_list;
    wait_list = waiter->next;

    // 唤醒该进程
    // 遍历进程链表查找指定PID的进程
    auto current = ProcessManager::get_current_task();
    if (!current) {
        delete waiter;
        return;
    }
    
    // 从当前进程开始遍历链表
    auto pcb = current;
    auto start = pcb;
    
    do {
        if (pcb->task_id == waiter->pid) {
            // 找到了指定PID的进程，将其状态设置为就绪
            pcb->state = PROCESS_READY;
            pcb->sleep_ticks = 0;
            break;
        }
        pcb = pcb->next;
    } while (pcb != start);

    // 释放等待节点
    delete waiter;
}

void Mutex::addWaiter() {
    // 获取当前进程
    auto current = ProcessManager::get_current_task();
    if (!current) {
        return;
    }

    // 创建新的等待节点
    WaitNode* node = new WaitNode();
    node->pid = current->task_id;
    node->next = nullptr;

    // 将节点添加到等待队列尾部
    if (!wait_list) {
        wait_list = node;
    } else {
        WaitNode* last = wait_list;
        while (last->next) {
            last = last->next;
        }
        last->next = node;
    }
}

void Mutex::removeWaiter(uint32_t pid) {
    // 如果等待队列为空，直接返回
    if (!wait_list) {
        return;
    }

    // 如果是队列头
    if (wait_list->pid == pid) {
        WaitNode* temp = wait_list;
        wait_list = wait_list->next;
        delete temp;
        return;
    }

    // 查找并移除节点
    WaitNode* prev = wait_list;
    WaitNode* curr = wait_list->next;
    while (curr) {
        if (curr->pid == pid) {
            prev->next = curr->next;
            delete curr;
            return;
        }
        prev = curr;
        curr = curr->next;
    }
}

bool Mutex::wouldDeadlock() const {
    // 简单的死锁检测：如果当前进程已经在等待队列中，则可能会导致死锁
    auto current = ProcessManager::get_current_task();
    if (!current) {
        return false;
    }

    uint32_t pid = current->task_id;
    WaitNode* node = wait_list;
    while (node) {
        if (node->pid == pid) {
            return true;
        }
        node = node->next;
    }

    return false;
}

} // namespace kernel