#pragma once

#include <cstdint>
#include <cstddef>
#include <arch/x86/spinlock.h>

// 日志缓冲区的最大容量
#define LOG_BUFFER_SIZE 4096
// 单条日志消息的最大长度
#define MAX_LOG_MESSAGE_SIZE 512

// 日志消息结构体
struct LogMessage {
    uint32_t cpu_id;       // 产生日志的CPU ID
    uint32_t timestamp;    // 时间戳
    uint32_t level;        // 日志级别
    char message[MAX_LOG_MESSAGE_SIZE]; // 日志内容
};

// 日志缓冲队列类
class LogBuffer {
private:
    // 私有构造函数，防止外部实例化
    LogBuffer();
    
    // 单例实例
    static LogBuffer* instance;
    
    // 环形缓冲区
    LogMessage buffer[LOG_BUFFER_SIZE];
    
    // 队列头尾指针
    uint32_t head;
    uint32_t tail;
    
    // 缓冲区锁
    SpinLock buffer_lock;
    
    // 消费者线程运行标志
    bool consumer_running;

public:
    // 获取单例实例
    static LogBuffer& get_instance();
    
    // 初始化日志缓冲队列
    void init();
    
    // 添加日志消息到队列
    bool enqueue(uint32_t level, const char* message);
    
    // 从队列中取出日志消息
    bool dequeue(LogMessage& message);
    
    // 启动日志消费线程
    void start_consumer();
    
    // 日志消费线程函数
    static void consumer_thread();
    
    // 获取队列中的消息数量
    uint32_t get_message_count();
    
    // 检查队列是否为空
    bool is_empty();
    
    // 检查队列是否已满
    bool is_full();
};

// 串口和控制台设备的互斥锁
extern SpinLock serial_lock;
extern SpinLock console_lock;