#include <lib/log_buffer.h>
#include <lib/debug.h>
#include <lib/serial.h>
#include <kernel/kernel.h>
#include <arch/x86/smp.h>
#include <kernel/process.h>
#include <cstring>

// 串口和控制台设备的互斥锁
SpinLock serial_lock;
SpinLock console_lock;

// 初始化单例指针
LogBuffer* LogBuffer::instance = nullptr;

// LogBuffer构造函数
LogBuffer::LogBuffer() : head(0), tail(0), consumer_running(false) {
    // 构造函数初始化成员变量
}

// 获取单例实例
LogBuffer& LogBuffer::get_instance() {
    if (instance == nullptr) {
        instance = new LogBuffer();
    }
    return *instance;
}

// 初始化日志缓冲队列
void LogBuffer::init() {
    head = 0;
    tail = 0;
    consumer_running = false;
    
    // 注册日志输出处理器
    LogOutputInterface log_handler = {
        [](LogLevel level, const char* message) {
            // 将日志消息添加到缓冲区
            LogBuffer::get_instance().enqueue(level, message);
        }
    };
    
    set_log_output_handler(log_handler);
}

// 添加日志消息到队列
bool LogBuffer::enqueue(uint32_t level, const char* message) {
    if (!message) {
        return false;
    }
    
    // 获取锁
    uint32_t flags;
    buffer_lock.acquire_irqsave(flags);
    
    // 检查队列是否已满
    if (is_full()) {
        buffer_lock.release_irqrestore(flags);
        return false;
    }
    
    // 填充消息结构
    buffer[tail].cpu_id = arch::smp_get_current_cpu();
    buffer[tail].timestamp = Kernel::instance().get_ticks();
    buffer[tail].level = level;
    
    // 复制消息内容，确保不会溢出
    strncpy(buffer[tail].message, message, MAX_LOG_MESSAGE_SIZE - 1);
    buffer[tail].message[MAX_LOG_MESSAGE_SIZE - 1] = '\0';
    
    // 更新尾指针
    tail = (tail + 1) % LOG_BUFFER_SIZE;
    
    // 释放锁
    buffer_lock.release_irqrestore(flags);
    
    return true;
}

// 从队列中取出日志消息
bool LogBuffer::dequeue(LogMessage& message) {
    // 获取锁
    uint32_t flags;
    buffer_lock.acquire_irqsave(flags);
    
    // 检查队列是否为空
    if (is_empty()) {
        buffer_lock.release_irqrestore(flags);
        return false;
    }
    
    // 复制消息
    message = buffer[head];
    
    // 更新头指针
    head = (head + 1) % LOG_BUFFER_SIZE;
    
    // 释放锁
    buffer_lock.release_irqrestore(flags);
    
    return true;
}

// 检查队列是否为空
bool LogBuffer::is_empty() {
    return head == tail;
}

// 检查队列是否已满
bool LogBuffer::is_full() {
    return ((tail + 1) % LOG_BUFFER_SIZE) == head;
}

// 获取队列中的消息数量
uint32_t LogBuffer::get_message_count() {
    uint32_t flags;
    buffer_lock.acquire_irqsave(flags);
    
    uint32_t count;
    if (tail >= head) {
        count = tail - head;
    } else {
        count = LOG_BUFFER_SIZE - head + tail;
    }
    
    buffer_lock.release_irqrestore(flags);
    return count;
}

// 启动日志消费线程
void LogBuffer::start_consumer() {
    if (!consumer_running) {
        consumer_running = true;
        
        // 创建内核线程作为日志消费者
        ProcessManager::kernel_thread("log_consumer", (uint32_t)LogBuffer::consumer_thread, 0, nullptr);
    }
}

// 日志消费线程函数
void LogBuffer::consumer_thread() {
    LogBuffer& log_buffer = LogBuffer::get_instance();
    LogMessage message;
    
    while (log_buffer.consumer_running) {
        // 尝试从队列中取出消息
        if (log_buffer.dequeue(message)) {
            // 获取锁以确保输出不会交错
            uint32_t flags;
            serial_lock.acquire_irqsave(flags);
            
            // 根据日志级别选择颜色
            if (message.level <= LOG_ERR) {
                // 错误级别消息同时输出到控制台
                console_lock.acquire();
                Console::print(message.message);
                console_lock.release();
            }
            
            // 输出到串口
            serial_puts(message.message);
            
            // 释放锁
            serial_lock.release_irqrestore(flags);
        } else {
            // 队列为空，让出CPU
            // 在实际系统中，这里可以使用一个条件变量或信号量来等待
            // 简单起见，我们只是让出CPU时间片
            asm volatile("pause");
        }
    }
}