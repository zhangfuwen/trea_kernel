#pragma once
#ifndef DEBUG_H
#define DEBUG_H

#include "kernel/process.h"
#include <cstdarg>
#include <lib/console.h>
#include <lib/string.h>
#include <cstdint>

// 定义日志级别
enum LogLevel {
    LOG_EMERG = 0,   // 系统不可用
    LOG_ALERT = 1,   // 必须立即采取行动
    LOG_CRIT = 2,    // 临界条件
    LOG_ERR = 3,     // 错误条件
    LOG_WARNING = 4, // 警告条件
    LOG_NOTICE = 5,  // 正常但重要的条件
    LOG_INFO = 6,    // 信息性消息
    LOG_DEBUG = 7    // 调试级别消息
};

// 当前日志级别，可以根据需要调整
extern LogLevel current_log_level;

// 日志级别对应的颜色
const uint8_t log_level_colors[] = {
    VGA_COLOR_LIGHT_RED,     // EMERG
    VGA_COLOR_RED,           // ALERT
    VGA_COLOR_LIGHT_MAGENTA, // CRIT
    VGA_COLOR_MAGENTA,       // ERR
    VGA_COLOR_LIGHT_BROWN,   // WARNING
    VGA_COLOR_CYAN,          // NOTICE
    VGA_COLOR_LIGHT_BLUE,    // INFO
    VGA_COLOR_LIGHT_GREY     // DEBUG
};

// 日志级别前缀
inline const char* log_level_prefix[] = {
    "<0>", // EMERG
    "<1>", // ALERT
    "<2>", // CRIT
    "<3>", // ERR
    "<4>", // WARNING
    "<5>", // NOTICE
    "<6>", // INFO
    "<7>"  // DEBUG
};

// ... 保留前面的枚举和变量定义 ...

// 修改后的format_string_v函数

int format_string_v(char* buffer, size_t size, const char* format, va_list args);
int format_string(char* buffer, size_t size, const char* format, ...);

const char* get_filename_from_path(const char* path);

struct LogOutputInterface {
    void (*print)(LogLevel level, const char* message);
};

extern LogOutputInterface log_output_handler;

// 核心打印函数调整
inline void _debug_print(
    LogLevel level, const char* file, int line, const char* func, const char* format, ...)
{
    if(level > current_log_level)
        return;

    char msg_buffer[512];
    int len = 0;
#ifndef NO_PID
    // 获取当前进程PID
    int pid = 0;
    ProcessControlBlock* current = ProcessManager::get_current_process();
    if(current) {
        pid = current->pid;
    }
    // 打印PID、文件名、行号和函数名
    len = format_string(msg_buffer, sizeof(msg_buffer),
        "%s[PID:%d] (%s:%d %s): ", log_level_prefix[current_log_level], pid,
        get_filename_from_path(file), line, func);
#else
    char info_buffer[256];
    len = format_string(msg_buffer, sizeof(msg_buffer),
        "%s %s:%d %s(): ", log_level_prefix[current_log_level], get_filename_from_path(file), line, func);
#endif

    va_list args;
    va_start(args, format);
    format_string_v(msg_buffer + len, sizeof(msg_buffer) - len, format, args);
    va_end(args);

    log_output_handler.print(level, msg_buffer);
}

// 新增接口设置函数（需在cpp文件中实现）
void set_log_output_handler(const LogOutputInterface& new_handler);

// 设置当前日志级别
inline void set_log_level(LogLevel level)
{
    current_log_level = level;
}
void hexdump(const void* buf, size_t size);

// 定义各种日志级别的宏
#define debug_emerg(fmt, ...)                                                                      \
    _debug_print(LOG_EMERG, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define debug_alert(fmt, ...)                                                                      \
    _debug_print(LOG_ALERT, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define debug_crit(fmt, ...)                                                                       \
    _debug_print(LOG_CRIT, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define debug_err(fmt, ...) _debug_print(LOG_ERR, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define debug_warn(fmt, ...)                                                                       \
    _debug_print(LOG_WARNING, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define debug_notice(fmt, ...)                                                                     \
    _debug_print(LOG_NOTICE, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define debug_info(fmt, ...)                                                                       \
    _debug_print(LOG_INFO, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define debug_debug(fmt, ...)                                                                      \
    _debug_print(LOG_DEBUG, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

// 类似printk的函数
#define printk(fmt, ...) _debug_print(LOG_INFO, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
// ... 其他日志宏定义保持不变 ...

// 新增速率限制调试宏（每秒最多打印1次）
#define debug_rate_limited(fmt, ...)                                                               \
    do {                                                                                           \
        static uint32_t last_print = 0;                                                            \
        static constexpr uint32_t interval_ticks = 100; /* 100 ticks = 1秒（假设1tick=10ms） */    \
        uint32_t current_tick = Kernel::instance().get_ticks();                                    \
        if(current_tick - last_print > interval_ticks) {                                           \
            _debug_print(LOG_INFO, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__);              \
            last_print = current_tick;                                                             \
        }                                                                                          \
    } while(0)

// 增强版（可自定义级别和间隔）
#define debug_rate_limited_ex(level, ms_interval, fmt, ...)                                        \
    do {                                                                                           \
        static uint32_t last_print##__LINE__ = 0;                                                  \
        const uint32_t interval_ticks = (ms_interval) / 10;                                        \
        uint32_t current_tick = Kernel::instance().get_ticks();                                    \
        if(current_tick - last_print##__LINE__ > interval_ticks) {                                 \
            _debug_print(level, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__);                 \
            last_print##__LINE__ = current_tick;                                                   \
        }                                                                                          \
    } while(0)

void hexdump(const void* buf, size_t size, void(*)(const char *));


#endif // DEBUG_H