#pragma once

#include <lib/console.h>
#include <lib/string.h>
#include <cstdarg>

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

// 简单的格式化函数，支持%d, %x, %s, %c
inline void format_string(char* buffer, size_t size, const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    size_t i = 0;
    const char* p = format;
    
    while (*p && i < size - 1) {
        if (*p != '%') {
            buffer[i++] = *p++;
            continue;
        }
        
        p++; // 跳过%
        if (!*p) break;
        
        switch (*p) {
            case 'd': { // 十进制整数
                int val = va_arg(args, int);
                char num_buffer[32];
                int j = 0;
                
                if (val < 0) {
                    buffer[i++] = '-';
                    val = -val;
                }
                
                do {
                    num_buffer[j++] = '0' + (val % 10);
                    val /= 10;
                } while (val && j < 31);
                
                while (j > 0 && i < size - 1) {
                    buffer[i++] = num_buffer[--j];
                }
                break;
            }
            case 'x': { // 十六进制整数
                unsigned int val = va_arg(args, unsigned int);
                char num_buffer[32];
                int j = 0;
                
                do {
                    int digit = val % 16;
                    num_buffer[j++] = digit < 10 ? '0' + digit : 'a' + digit - 10;
                    val /= 16;
                } while (val && j < 31);
                
                while (j > 0 && i < size - 1) {
                    buffer[i++] = num_buffer[--j];
                }
                break;
            }
            case 's': { // 字符串
                const char* str = va_arg(args, const char*);
                if (!str) str = "(null)";
                
                while (*str && i < size - 1) {
                    buffer[i++] = *str++;
                }
                break;
            }
            case 'c': { // 字符
                char c = (char)va_arg(args, int);
                buffer[i++] = c;
                break;
            }
            default: // 未知格式，原样输出
                buffer[i++] = *p;
                break;
        }
        
        p++;
    }
    
    buffer[i] = '\0';
    va_end(args);
}

// 接受va_list参数的格式化函数
inline void format_string_v(char* buffer, size_t size, const char* format, va_list args) {
    size_t i = 0;
    const char* p = format;
    
    while (*p && i < size - 1) {
        if (*p != '%') {
            buffer[i++] = *p++;
            continue;
        }
        
        p++; // 跳过%
        if (!*p) break;
        
        switch (*p) {
            case 'd': { // 十进制整数
                int val = va_arg(args, int);
                char num_buffer[32];
                int j = 0;
                
                if (val < 0) {
                    buffer[i++] = '-';
                    val = -val;
                }
                
                do {
                    num_buffer[j++] = '0' + (val % 10);
                    val /= 10;
                } while (val && j < 31);
                
                while (j > 0 && i < size - 1) {
                    buffer[i++] = num_buffer[--j];
                }
                break;
            }
            case 'x': { // 十六进制整数
                unsigned int val = va_arg(args, unsigned int);
                char num_buffer[32];
                int j = 0;
                
                do {
                    int digit = val % 16;
                    num_buffer[j++] = digit < 10 ? '0' + digit : 'a' + digit - 10;
                    val /= 16;
                } while (val && j < 31);
                
                while (j > 0 && i < size - 1) {
                    buffer[i++] = num_buffer[--j];
                }
                break;
            }
            case 's': { // 字符串
                const char* str = va_arg(args, const char*);
                if (!str) str = "(null)";
                
                while (*str && i < size - 1) {
                    buffer[i++] = *str++;
                }
                break;
            }
            case 'c': { // 字符
                char c = (char)va_arg(args, int);
                buffer[i++] = c;
                break;
            }
            default: // 未知格式，原样输出
                buffer[i++] = *p;
                break;
        }
        
        p++;
    }
    
    buffer[i] = '\0';
}

// 核心打印函数
inline void _debug_print(LogLevel level, const char* file, int line, const char* func, const char* format, ...) {
    if (level > current_log_level) return;
    
    // 保存当前颜色
    uint8_t old_color = VGA_COLOR_LIGHT_GREY;
    
    // 设置日志级别对应的颜色
    Console::setColor(log_level_colors[level], VGA_COLOR_BLACK);
    
    // 打印日志级别前缀
    Console::print(log_level_prefix[level]);
    
    // 打印文件名、行号和函数名
    char info_buffer[256];
    format_string(info_buffer, sizeof(info_buffer), " %s:%d %s(): ", file, line, func);
    Console::print(info_buffer);
    
    // 格式化并打印用户消息
    char msg_buffer[512];
    va_list args;
    va_start(args, format);
    
    // 使用接受va_list的格式化函数
    format_string_v(msg_buffer, sizeof(msg_buffer), format, args);
    va_end(args);
    
    Console::print(msg_buffer);
    Console::print("\n");
    
    // 恢复颜色
    Console::setColor(old_color, VGA_COLOR_BLACK);
}

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