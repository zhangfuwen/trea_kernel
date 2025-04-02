#include "lib/test_framework.h"
#include <cstdarg>
// #include <cstring>
#define NO_PID
#include <lib/debug.h>

// 声明format_string_v函数
extern int format_string_v(char* buffer, size_t size, const char* format, va_list args);

// // 包装函数，方便测试
// int format_string(char* buffer, size_t size, const char* format, ...) {
//     va_list args;
//     va_start(args, format);
//     int result = format_string_v(buffer, size, format, args);
//     va_end(args);
//     return result;
// }

TEST_CASE(basic_hex_formatting) {
    char buffer[100];
    
    // 简单的十六进制值
    format_string(buffer, sizeof(buffer), "%x", 255);
    ASSERT_STR_EQ("ff", buffer);
    
    // 指定宽度的十六进制
    format_string(buffer, sizeof(buffer), "%4x", 255);
    ASSERT_STR_EQ("  ff", buffer);
    
    // 零填充的十六进制
    format_string(buffer, sizeof(buffer), "%04x", 255);
    ASSERT_STR_EQ("00ff", buffer);
    
    // 带前缀的十六进制
    format_string(buffer, sizeof(buffer), "%#x", 255);
    ASSERT_STR_EQ("0xff", buffer);
}

TEST_CASE(advanced_hex_formatting) {
    char buffer[100];
    
    // 左对齐的十六进制
    format_string(buffer, sizeof(buffer), "%-4x", 255);
    ASSERT_STR_EQ("ff  ", buffer);
    
    // 大写十六进制
    format_string(buffer, sizeof(buffer), "%X", 255);
    ASSERT_STR_EQ("FF", buffer);
    
    // 组合标志
    format_string(buffer, sizeof(buffer), "%#08x", 255);
    ASSERT_STR_EQ("0x0000ff", buffer);
}

TEST_CASE(edge_cases) {
    char buffer[100];
    
    // 零值
    format_string(buffer, sizeof(buffer), "%x", 0);
    ASSERT_STR_EQ("0", buffer);
    
    // 最大32位值
    format_string(buffer, sizeof(buffer), "%x", 0xFFFFFFFF);
    ASSERT_STR_EQ("ffffffff", buffer);
    
    // 小缓冲区测试
    char small_buffer[3];
    int result = format_string(small_buffer, sizeof(small_buffer), "%x", 255);
    ASSERT_EQ(2, result);
    ASSERT_STR_EQ("ff", small_buffer);
}

TEST_CASE(hexdump_formatting) {
    char buffer[100];
    uint8_t test_data[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    
    // 测试地址格式化
    format_string(buffer, sizeof(buffer), "%08zx", (size_t)0x1234);
    ASSERT_STR_EQ("00001234", buffer);
    
    // 测试十六进制字节格式化
    format_string(buffer, sizeof(buffer), "%02x", test_data[5]);
    ASSERT_STR_EQ("55", buffer);
    
    // 测试组合格式化（模拟hexdump中的地址部分）
    format_string(buffer, sizeof(buffer), "%08zx:", (size_t)0x1000);
    ASSERT_STR_EQ("00001000:", buffer);
    
    // 测试大数值格式化
    format_string(buffer, sizeof(buffer), "%08zx", (size_t)0xFFFFFFFF);
    ASSERT_STR_EQ("ffffffff", buffer);
    
    // 测试零值格式化
    format_string(buffer, sizeof(buffer), "%08zx", (size_t)0);
    ASSERT_STR_EQ("00000000", buffer);
}

int main() {
    RUN_TEST(basic_hex_formatting);
    RUN_TEST(advanced_hex_formatting);
    RUN_TEST(edge_cases);
    RUN_TEST(hexdump_formatting);
    
    print_test_results();
    return g_test_stats.failed_tests;
}