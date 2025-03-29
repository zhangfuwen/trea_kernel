#include "lib/debug.h"


LogLevel current_log_level = LOG_DEBUG;

int format_string_v(char* buffer, size_t size, const char* format, va_list args) {
    static const char digits[] = "0123456789abcdef";
    char* p = buffer;
    const char* end = buffer + size;

    for (; *format && p < end; format++) {
        if (*format != '%') {
            *p++ = *format;
            continue;
        }

        // 解析格式说明符
        int width = 0;
        int prec = -1;
        bool alt = false;
        bool zero_pad = false;
        bool left_align = false;
        char type = 0;

        // 解析标志位
        for (format++; *format; format++) {
            switch (*format) {
                case '#': alt = true; break;
                case '0': zero_pad = true; break;
                case '-': left_align = true; break;
                default: goto parse_width;
            }
        }

    parse_width:
        // 解析宽度
        if (*format >= '0' && *format <= '9') {
            for (width = 0; *format >= '0' && *format <= '9'; format++)
                width = width * 10 + (*format - '0');
            format--;
        } else if (*format == '*') {
            width = va_arg(args, int);
            format++;
        }

        // 解析精度
        if (*format == '.') {
            format++;
            prec = 0;
            if (*format >= '0' && *format <= '9') {
                for (prec = 0; *format >= '0' && *format <= '9'; format++)
                    prec = prec * 10 + (*format - '0');
                format--;
            } else if (*format == '*') {
                prec = va_arg(args, int);
                format++;
            }
        }

        // 解析长度修饰符
        if (*format == 'l') {
            format++;
            if (*format == 'l') format++; // 支持ll
        } else if (*format == 'h') {
            format++;
            if (*format == 'h') format++; // 支持hh
        }

        type = *format;
        switch (type) {
        case 'd':
        case 'i': {
            long val = va_arg(args, long);
            char num_buf[32];
            char* q = num_buf;
            int is_neg = 0;

            if (val < 0) {
                is_neg = 1;
                val = -val;
            }

            do {
                *q++ = digits[val % 10];
                val /= 10;
            } while (val);

            // 处理宽度和填充
            int len = q - num_buf + is_neg;
            if (!left_align && width > len) {
                for (int i = len; i < width && p < end; i++)
                    *p++ = zero_pad ? '0' : ' ';
            }

            if (is_neg && p < end) *p++ = '-';

            while (q != num_buf && p < end)
                *p++ = *--q;

            if (left_align && width > len) {
                for (int i = len; i < width && p < end; i++)
                    *p++ = ' ';
            }
            break;
        }
        case 'u': {
            unsigned long val = va_arg(args, unsigned long);
            char num_buf[32];
            char* q = num_buf;

            do {
                *q++ = digits[val % 10];
                val /= 10;
            } while (val);

            // 处理宽度和填充
            int len = q - num_buf;
            if (!left_align && width > len) {
                for (int i = len; i < width && p < end; i++)
                    *p++ = zero_pad ? '0' : ' ';
            }

            while (q != num_buf && p < end)
                *p++ = *--q;

            if (left_align && width > len) {
                for (int i = len; i < width && p < end; i++)
                    *p++ = ' ';
            }
            break;
        }
        case 'x':
        case 'X': {
            unsigned long val = va_arg(args, unsigned long);
            char num_buf[32];
            char* q = num_buf;
            int prefix_len = 0;

            // 计算前缀长度(0x)
            if (alt) {
                prefix_len = 2;
            }

            // 生成数字字符串
            do {
                *q++ = digits[val % 16];
                val /= 16;
            } while (val);

            // 计算实际数字长度
            int num_len = q - num_buf;
            int total_len = num_len + prefix_len;

            // 处理宽度和填充
            if (!left_align && width > total_len) {
                // 零填充时，前缀要放在最前面
                if (zero_pad && prefix_len > 0 && p + prefix_len < end) {
                    *p++ = '0';
                    *p++ = 'x';
                    prefix_len = 0; // 已经输出前缀
                }

                // 填充剩余宽度
                for (int i = total_len; i < width && p < end; i++) {
                    *p++ = zero_pad ? '0' : ' ';
                }
            }

            // 输出前缀(如果还没输出)
            if (prefix_len > 0 && p + 1 < end) {
                *p++ = '0';
                *p++ = 'x';
            }

            // 输出数字
            while (q != num_buf && p < end) {
                *p++ = *--q;
            }

            // 左对齐时的填充
            if (left_align && width > total_len) {
                for (int i = total_len; i < width && p < end; i++) {
                    *p++ = ' ';
                }
            }
            break;
        }
        case 'o': {
            unsigned long val = va_arg(args, unsigned long);
            char num_buf[32];
            char* q = num_buf;

            if (alt && p < end) *p++ = '0';

            do {
                *q++ = digits[val % 8];
                val /= 8;
            } while (val);

            // 处理宽度和填充
            int len = q - num_buf + (alt ? 1 : 0);
            if (!left_align && width > len) {
                for (int i = len; i < width && p < end; i++)
                    *p++ = zero_pad ? '0' : ' ';
            }

            while (q != num_buf && p < end)
                *p++ = *--q;

            if (left_align && width > len) {
                for (int i = len; i < width && p < end; i++)
                    *p++ = ' ';
            }
            break;
        }
        case 's': {
            const char* str = va_arg(args, const char*);
            if (!str) str = "(null)";

            int len = strlen(str);
            if (prec >= 0 && prec < len) len = prec;

            if (!left_align && width > len) {
                for (int i = len; i < width && p < end; i++)
                    *p++ = ' ';
            }

            for (int i = 0; i < len && *str && p < end; i++)
                *p++ = *str++;

            if (left_align && width > len) {
                for (int i = len; i < width && p < end; i++)
                    *p++ = ' ';
            }
            break;
        }
        case 'c':
            if (p < end) *p++ = (char)va_arg(args, int);
            break;
        case 'p': {
            void* ptr = va_arg(args, void*);
            unsigned long val = (unsigned long)ptr;

            if (p < end) *p++ = '0';
            if (p < end) *p++ = 'x';

            char num_buf[32];
            char* q = num_buf;

            do {
                *q++ = digits[val % 16];
                val /= 16;
            } while (val);

            while (q != num_buf && p < end)
                *p++ = *--q;
            break;
        }
        case '%':
            if (p < end) *p++ = '%';
            break;
        default:
            if (p < end) *p++ = *format;
            break;
        }
    }

    if (p < end) *p = '\0';
    else if (size > 0) buffer[size-1] = '\0';

    return p - buffer;
}
void format_string(char* buffer, size_t size, const char* format, ...) {
    va_list args;
    va_start(args, format);
    format_string_v(buffer, size, format, args);
    va_end(args);
}
void _debug_print(LogLevel level, const char* file, int line, const char* func, const char* format, ...) {
    if (level > current_log_level) return;

    // 保存当前颜色
    uint8_t old_color = VGA_COLOR_LIGHT_GREY;

    // 设置日志级别对应的颜色
    Console::setColor(log_level_colors[level], VGA_COLOR_BLACK);

    // 打印日志级别前缀
    Console::print(log_level_prefix[level]);

#ifndef NO_PID
    // 获取当前进程PID
    int pid = 0;
    ProcessControlBlock* current = ProcessManager::get_current_process();
    if (current) {
        pid = current->pid;
    }

    // 打印PID、文件名、行号和函数名
    char info_buffer[256];
    format_string(info_buffer, sizeof(info_buffer), "[PID:%d] %s:%d %s(): ", pid, file, line, func);
    Console::print(info_buffer);
#else
    char info_buffer[256];
    format_string(info_buffer, sizeof(info_buffer), "%s:%d %s(): ", file, line, func);
    Console::print(info_buffer);

#endif

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
