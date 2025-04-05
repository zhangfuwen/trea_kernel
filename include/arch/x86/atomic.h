#pragma once

#include <cstdint>

namespace arch {

// 内存顺序模式
enum memory_order {
    relaxed = __ATOMIC_RELAXED,
    consume = __ATOMIC_CONSUME,
    acquire = __ATOMIC_ACQUIRE,
    release = __ATOMIC_RELEASE,
    acq_rel = __ATOMIC_ACQ_REL,
    seq_cst = __ATOMIC_SEQ_CST
};

template<typename T>
inline T atomic_load(const volatile T* ptr, memory_order order = seq_cst) {
    T value;
    __atomic_load(ptr, &value, order);
    return value;
}

template<typename T>
inline void atomic_store(volatile T* ptr, T value, memory_order order = seq_cst) {
    __atomic_store(ptr, &value, order);
}

template<typename T>
inline bool atomic_compare_exchange(volatile T* ptr, T* expected, T desired,
                                   memory_order success = seq_cst,
                                   memory_order failure = seq_cst) {
    return __atomic_compare_exchange(ptr, expected, &desired, false,
                                    success, failure);
}

// x86架构特定的原子操作实现
inline uint32_t atomic_add(volatile uint32_t* ptr, uint32_t value) {
    asm volatile("lock xadd %0, %1"
                : "+r"(value), "+m"(*ptr)
                : 
                : "memory");
    return value;
}

inline uint64_t atomic_add64(volatile uint64_t* ptr, uint64_t value) {
    asm volatile("lock xaddq %0, %1"
                : "+r"(value), "+m"(*ptr)
                : 
                : "memory");
    return value;
}

inline void memory_barrier() {
    asm volatile("mfence" ::: "memory");
}

} // namespace arch