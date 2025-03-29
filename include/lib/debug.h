#pragma once
#ifndef DEBUG_H
#define DEBUG_H

#include <lib/console.h>
#include <lib/string.h>
#include <cstdarg>
#include "kernel/process.h"

// 定义日志级别
enum LogLevel {
    LOG_EMERG = 0,    // 系统不可用
    LOG_ALERT = 1,    // 必须立即采取行动
    LOG_CRIT = 2,     // 临界条件
    LOG_ERR = 3,      // 错误条件
    LOG_WARNING = 4,  // 警告条件
    LOG_NOTICE = 5,   // 正常但重要的条件
    LOG_INFO = 6,     // 信息性消息
    LOG_DEBUG = 7     // 调试级别消息
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
// 修改后的format_string函数
void format_string(char* buffer, size_t size, const char* format, ...);

// 核心打印函数
void _debug_print(LogLevel level, const char* file, int line, const char* func, const char* format, ...);
// 设置当前日志级别
inline void set_log_level(LogLevel level) {
    current_log_level = level;
}

// 定义各种日志级别的宏
#define debug_emerg(fmt, ...) _debug_print(LOG_EMERG, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define debug_alert(fmt, ...) _debug_print(LOG_ALERT, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define debug_crit(fmt, ...)  _debug_print(LOG_CRIT, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define debug_err(fmt, ...)   _debug_print(LOG_ERR, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define debug_warn(fmt, ...)  _debug_print(LOG_WARNING, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define debug_notice(fmt, ...) _debug_print(LOG_NOTICE, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define debug_info(fmt, ...)  _debug_print(LOG_INFO, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define debug_debug(fmt, ...) _debug_print(LOG_DEBUG, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

// 类似printk的函数
#define printk(fmt, ...) _debug_print(LOG_INFO, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#endif // DEBUG_H