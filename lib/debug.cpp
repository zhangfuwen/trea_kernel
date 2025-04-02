#include "lib/debug.h"
#include "lib/serial.h"
#include <cstdint>

LogLevel current_log_level = LOG_DEBUG;

const char* get_filename_from_path(const char* path)
{
    const char* last_slash = strrchr(path, '/');
    return (last_slash != nullptr) ? (last_slash + 1) : path;
}

// 初始化串口
static bool serial_initialized = false;
static void ensure_serial_initialized() {
    if (!serial_initialized) {
        serial_init();
        serial_initialized = true;
    }
}

// 默认输出处理器（使用串口）
LogOutputInterface log_output_handler = {
    [](LogLevel level, const char* message) {
        ensure_serial_initialized();
        serial_puts(message);
    }
};

// 移除颜色保存/恢复相关代码

void set_log_output_handler(const LogOutputInterface& new_handler) {
    log_output_handler = new_handler;
}

// 格式化标志结构体
struct FormatFlags {
    int width = 0;
    int precision = -1;
    bool alternative = false;
    bool zero_padding = false;
    bool left_align = false;
    char type = 0;
};

// 解析格式说明符
const char* parse_format_flags(const char* format, FormatFlags& flags, va_list& args)
{
    // 解析标志位
    while (*format) {
        switch(*format) {
            case '#':
                flags.alternative = true;
                format++;
                continue;
            case '0':
                flags.zero_padding = true;
                format++;
                continue;
            case '-':
                flags.left_align = true;
                format++;
                continue;
            default:
                break;
        }
        break;
    }

    // 解析宽度
    if(*format >= '0' && *format <= '9') {
        for(flags.width = 0; *format >= '0' && *format <= '9'; format++)
            flags.width = flags.width * 10 + (*format - '0');
    } else if(*format == '*') {
        flags.width = va_arg(args, int);
        format++;
    }

    // 解析精度
    if(*format == '.') {
        format++;
        flags.precision = 0;
        if(*format >= '0' && *format <= '9') {
            for(flags.precision = 0; *format >= '0' && *format <= '9'; format++)
                flags.precision = flags.precision * 10 + (*format - '0');
        } else if(*format == '*') {
            flags.precision = va_arg(args, int);
            format++;
        }
    }

    // 解析长度修饰符
    if(*format == 'l') {
        format++;
        if(*format == 'l')
            format++; // 支持ll
    } else if(*format == 'h') {
        format++;
        if(*format == 'h')
            format++; // 支持hh
    } else if(*format == 'z') {
        format++; // 支持z (size_t)
    }

    flags.type = *format;
    return format;
}

// 处理填充和对齐
char* apply_padding(char* p, const char* end, int content_len, const FormatFlags& flags, bool prefix_handled = false)
{
    // 计算需要填充的空格数量
    int padding = (flags.width > content_len) ? flags.width - content_len : 0;
    
    // 右对齐时，先填充空格或零
    if (!flags.left_align && padding > 0) {
        char pad_char = (flags.zero_padding && !prefix_handled) ? '0' : ' ';
        for (int i = 0; i < padding && p < end; i++) {
            *p++ = pad_char;
        }
    }
    
    return p;
}

// 处理后置填充（左对齐）
char* apply_post_padding(char* p, const char* end, int content_len, const FormatFlags& flags)
{
    if (flags.left_align && flags.width > content_len) {
        int padding = flags.width - content_len;
        for (int i = 0; i < padding && p < end; i++) {
            *p++ = ' ';
        }
    }
    return p;
}

// 处理整数格式化
char* format_integer(char* p, const char* end, long value, const FormatFlags& flags)
{
    static const char digits[] = "0123456789abcdef";
    char num_buf[32];
    char* q = num_buf;
    int is_neg = 0;
    long val = value;

    if(val < 0) {
        is_neg = 1;
        val = -val;
    }

    // 生成数字字符串（反向）
    do {
        *q++ = digits[val % 10];
        val /= 10;
    } while(val);

    // 计算总长度（包括符号）
    int len = q - num_buf + is_neg;
    
    // 应用填充（右对齐）
    if (!flags.left_align) {
        if (flags.zero_padding) {
            // 如果是零填充，先输出符号
            if (is_neg && p < end) {
                *p++ = '-';
                is_neg = 0; // 标记符号已处理
            }
        }
        p = apply_padding(p, end, len, flags);
    }
    
    // 输出符号（如果还没输出）
    if (is_neg && p < end) {
        *p++ = '-';
    }

    // 输出数字
    while (q != num_buf && p < end) {
        *p++ = *--q;
    }

    // 应用后置填充（左对齐）
    p = apply_post_padding(p, end, len, flags);
    
    return p;
}

// 处理无符号整数格式化
char* format_unsigned(char* p, const char* end, unsigned long value, const FormatFlags& flags)
{
    static const char digits[] = "0123456789abcdef";
    char num_buf[32];
    char* q = num_buf;
    unsigned long val = value;

    // 生成数字字符串（反向）
    do {
        *q++ = digits[val % 10];
        val /= 10;
    } while(val);

    // 计算长度
    int len = q - num_buf;
    
    // 应用填充（右对齐）
    p = apply_padding(p, end, len, flags);

    // 输出数字
    while (q != num_buf && p < end) {
        *p++ = *--q;
    }

    // 应用后置填充（左对齐）
    p = apply_post_padding(p, end, len, flags);
    
    return p;
}

// 处理十六进制格式化
char* format_hex(char* p, const char* end, unsigned long value, const FormatFlags& flags)
{
    static const char lower_digits[] = "0123456789abcdef";
    static const char upper_digits[] = "0123456789ABCDEF";
    
    const char* hex_digits = (flags.type == 'X') ? upper_digits : lower_digits;
    char num_buf[32];
    int num_len = 0;
    unsigned long val = value;

    // 生成数字字符串
    if (val == 0) {
        num_buf[num_len++] = '0';
    } else {
        // 先生成反向的数字字符串
        char temp_buf[32];
        int temp_len = 0;
        do {
            temp_buf[temp_len++] = hex_digits[val % 16];
            val /= 16;
        } while(val);
        
        // 反转到最终缓冲区
        for (int i = temp_len - 1; i >= 0; i--) {
            num_buf[num_len++] = temp_buf[i];
        }
    }
    num_buf[num_len] = '\0';

    // 计算前缀长度和总长度
    int prefix_len = (flags.alternative) ? 2 : 0;
    int total_len = num_len;

    // 处理精度（最小数字位数）
    int zero_padding = 0;
    if (flags.precision > num_len) {
        zero_padding = flags.precision - num_len;
        total_len = flags.precision;
    } else if (flags.zero_padding && !flags.left_align && flags.width > (total_len + prefix_len)) {
        zero_padding = flags.width - total_len - prefix_len;
    }

    total_len += prefix_len + zero_padding;

    // 计算空格填充数量
    int space_padding = (flags.width > total_len) ? flags.width - total_len : 0;

    // 输出空格（右对齐时）
    if (!flags.left_align && space_padding > 0) {
        for (int i = 0; i < space_padding && p < end; i++) {
            *p++ = ' ';
        }
    }

    // 输出前缀
    if (flags.alternative && p < end - 1) {
        *p++ = '0';
        *p++ = (flags.type == 'X') ? 'X' : 'x';
    }

    // 输出前导零
    for (int i = 0; i < zero_padding && p < end; i++) {
        *p++ = '0';
    }

    // 输出数字
    for (int i = 0; i < num_len && p < end; i++) {
        *p++ = num_buf[i];
    }

    // 输出空格（左对齐时）
    if (flags.left_align && space_padding > 0) {
        for (int i = 0; i < space_padding && p < end; i++) {
            *p++ = ' ';
        }
    }
    
    return p;
}

// 处理八进制格式化
char* format_octal(char* p, const char* end, unsigned long value, const FormatFlags& flags)
{
    static const char digits[] = "01234567";
    char num_buf[32];
    char* q = num_buf;
    unsigned long val = value;
    bool prefix_handled = false;

    // 处理前缀
    if (flags.alternative) {
        if (p < end) {
            *p++ = '0';
            prefix_handled = true;
        }
    }

    // 生成数字字符串（反向）
    do {
        *q++ = digits[val % 8];
        val /= 8;
    } while(val);

    // 计算长度
    int len = q - num_buf + (flags.alternative ? 1 : 0);
    
    // 应用填充（右对齐）
    if (!prefix_handled) {
        p = apply_padding(p, end, len, flags);
    }

    // 输出数字
    while (q != num_buf && p < end) {
        *p++ = *--q;
    }

    // 应用后置填充（左对齐）
    p = apply_post_padding(p, end, len, flags);
    
    return p;
}

// 处理字符串格式化
char* format_string(char* p, const char* end, const char* str, const FormatFlags& flags)
{
    if (!str) {
        str = "(null)";
    }

    int len = strlen(str);
    if (flags.precision >= 0 && flags.precision < len) {
        len = flags.precision;
    }

    // 应用填充（右对齐）
    p = apply_padding(p, end, len, flags);

    // 输出字符串
    for (int i = 0; i < len && *str && p < end; i++) {
        *p++ = *str++;
    }

    // 应用后置填充（左对齐）
    p = apply_post_padding(p, end, len, flags);
    
    return p;
}

// 处理字符格式化
char* format_char(char* p, const char* end, int c, const FormatFlags& flags)
{
    // 应用填充（右对齐）
    p = apply_padding(p, end, 1, flags);

    // 输出字符
    if (p < end) {
        *p++ = (char)c;
    }

    // 应用后置填充（左对齐）
    p = apply_post_padding(p, end, 1, flags);
    
    return p;
}

// 处理指针格式化
char* format_pointer(char* p, const char* end, void* ptr)
{
    static const char digits[] = "0123456789abcdef";
    unsigned long val = (unsigned long)ptr;

    // 输出前缀
    if (p < end) {
        *p++ = '0';
    }
    if (p < end) {
        *p++ = 'x';
    }

    // 生成十六进制字符串
    char num_buf[32];
    char* q = num_buf;

    do {
        *q++ = digits[val % 16];
        val /= 16;
    } while(val);

    // 输出数字
    while (q != num_buf && p < end) {
        *p++ = *--q;
    }
    
    return p;
}

// 主格式化函数
int format_string_v(char* buffer, size_t size, const char* format, va_list args)
{
    char* p = buffer;
    const char* end = buffer + size;

    for(; *format && p < end; format++) {
        if(*format != '%') {
            *p++ = *format;
            continue;
        }

        // 解析格式说明符
        FormatFlags flags;
        format++; // 跳过%
        format = parse_format_flags(format, flags, args);

        // 根据类型调用相应的格式化函数
        switch(flags.type) {
            case 'd':
            case 'i':
                p = format_integer(p, end, va_arg(args, long), flags);
                break;
            case 'u':
                p = format_unsigned(p, end, va_arg(args, unsigned long), flags);
                break;
            case 'x':
            case 'X':
                p = format_hex(p, end, va_arg(args, unsigned long), flags);
                break;
            case 'o':
                p = format_octal(p, end, va_arg(args, unsigned long), flags);
                break;
            case 's':
                p = format_string(p, end, va_arg(args, const char*), flags);
                break;
            case 'c':
                p = format_char(p, end, va_arg(args, int), flags);
                break;
            case 'p':
                p = format_pointer(p, end, va_arg(args, void*));
                break;
            case '%':
                if(p < end) {
                    *p++ = '%';
                }
                break;
            default:
                if(p < end) {
                    *p++ = flags.type;
                }
                break;
        }
    }

    // 确保字符串以null结尾
    if(p < end) {
        *p = '\0';
    } else if(size > 0) {
        buffer[size - 1] = '\0';
    }

    return p - buffer;
}
int format_string(char* buffer, size_t size, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    int len = format_string_v(buffer, size, format, args);
    va_end(args);
    return len;
}

void hexdump(const void* buf, size_t size, void(printf_func)(const char*)) {
    const uint8_t* data = static_cast<const uint8_t*>(buf);
    size_t address = 0;
    char line[78]; // 8 + 1 + 8*3 + 3*2 + 16 + 1 = 78

    while (size > 0) {
        size_t line_size = (size > 16) ? 16 : size;
        char* p = line;

        // 地址部分 (00000000: )
        p += format_string(p, 9, "%08zx:", address);
        *p++ = ' ';

        // 十六进制部分
        for (size_t i = 0; i < line_size; i++) {
            p += format_string(p, 3, "%02x", data[i]);
            // 每8字节加空格分隔
            if (i == 7) *p++ = ' ';
            *p++ = ' ';
        }
        // 补齐剩余的十六进制部分
        for (size_t i = line_size; i < 16; i++) {
            p += format_string(p, 3, "  ");
            if (i == 7) *p++ = ' ';
            *p++ = ' ';
        }

        // ASCII部分
        *p++ = '|';
        *p++ = ' ';
        for (size_t i = 0; i < line_size; i++) {
            uint8_t c = data[i];
            *p++ = (c >= 32 && c <= 126) ? c : '.';
        }
        // 补齐剩余的 ASCII 部分
        for (size_t i = line_size; i < 16; i++) {
            *p++ = ' ';
        }
        *p = '\0';

        // 使用调试输出
        printf_func(line);

        data += line_size;
        address += line_size;
        size -= line_size;
    }
}




