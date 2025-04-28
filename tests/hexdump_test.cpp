#include "lib/test_framework.h"
#include <cstdarg>
#define NO_PID
#include <lib/debug.h>

// 测试用的输出捕获缓冲区
char captured_output[4096];
size_t captured_index = 0;

// 重置捕获缓冲区
void reset_capture() {
    memset(captured_output, 0, sizeof(captured_output));
    captured_index = 0;
}

// 捕获输出的函数
void capture_output(const char* line) {
    size_t len = strlen(line);
    if (captured_index + len < sizeof(captured_output) - 1) {
        strcpy(captured_output + captured_index, line);
        captured_index += len;
    }
}

// 测试基本功能
TEST_CASE(basic_hexdump) {
    reset_capture();
    
    // 创建一个包含0-15的简单数据块
    uint8_t test_data[16];
    for (int i = 0; i < 16; i++) {
        test_data[i] = i;
    }
    
    // 执行hexdump
    hexdump(test_data, sizeof(test_data), capture_output);
    
    // 验证输出格式
    const char* expected = "00000000: 00 01 02 03 04 05 06 07  08 09 0a 0b 0c 0d 0e 0f | ................";
    
    // 检查输出是否包含预期的格式
    bool contains_expected = (strstr(captured_output, expected) != nullptr);
    if (contains_expected) {
        g_test_stats.passed_tests++;
        printf("[PASS] %s:%d - Basic hexdump format\n", __FILE__, __LINE__);
    } else {
        g_test_stats.failed_tests++;
        printf("[FAIL] %s:%d - Expected format not found\nExpected: %s\nGot: %s\n", 
               __FILE__, __LINE__, expected, captured_output);
    }
    g_test_stats.total_tests++;
}

// 测试ASCII显示
TEST_CASE(ascii_display) {
    reset_capture();
    
    // 创建一个包含可打印和不可打印字符的数据块
    uint8_t test_data[16];
    for (int i = 0; i < 16; i++) {
        test_data[i] = i + 32; // 从空格(32)开始
    }
    
    // 执行hexdump
    hexdump(test_data, sizeof(test_data), capture_output);
    
    // 验证ASCII部分显示正确
    const char* expected_ascii = "| !\"#$%&'()*+,-./";
    
    // 检查输出是否包含预期的ASCII部分
    bool contains_expected = (strstr(captured_output, expected_ascii) != nullptr);
    if (contains_expected) {
        g_test_stats.passed_tests++;
        printf("[PASS] %s:%d - ASCII display\n", __FILE__, __LINE__);
    } else {
        g_test_stats.failed_tests++;
        printf("[FAIL] %s:%d - Expected ASCII not found\nExpected to contain: %s\nGot: %s\n", 
               __FILE__, __LINE__, expected_ascii, captured_output);
    }
    g_test_stats.total_tests++;
}

// 测试非可打印字符显示为点
TEST_CASE(nonprintable_chars) {
    reset_capture();
    
    // 创建一个包含不可打印字符的数据块
    uint8_t test_data[16];
    for (int i = 0; i < 16; i++) {
        test_data[i] = i; // 0-15都是不可打印字符
    }
    
    // 执行hexdump
    hexdump(test_data, sizeof(test_data), capture_output);
    
    // 验证不可打印字符显示为点
    const char* expected_dots = "| ................";
    
    // 检查输出是否包含预期的点
    bool contains_expected = (strstr(captured_output, expected_dots) != nullptr);
    if (contains_expected) {
        g_test_stats.passed_tests++;
        printf("[PASS] %s:%d - Non-printable chars display\n", __FILE__, __LINE__);
    } else {
        g_test_stats.failed_tests++;
        printf("[FAIL] %s:%d - Expected dots not found\nExpected to contain: %s\nGot: %s\n", 
               __FILE__, __LINE__, expected_dots, captured_output);
    }
    g_test_stats.total_tests++;
}

// 测试部分行（不足16字节）
TEST_CASE(partial_line) {
    reset_capture();
    
    // 创建一个只有10字节的数据块
    uint8_t test_data[10];
    for (int i = 0; i < 10; i++) {
        test_data[i] = 0x41 + i; // 从'A'开始
    }
    
    // 执行hexdump
    hexdump(test_data, sizeof(test_data), capture_output);
    
    // 验证输出格式 - 应该有填充空格
    const char* expected_hex = "00000000: 41 42 43 44 45 46 47 48  49 4a                   ";
    const char* expected_ascii = "| ABCDEFGHIJ";
    
    // 检查输出是否包含预期的格式
    bool contains_hex = (strstr(captured_output, expected_hex) != nullptr);
    bool contains_ascii = (strstr(captured_output, expected_ascii) != nullptr);
    
    if (contains_hex && contains_ascii) {
        g_test_stats.passed_tests++;
        printf("[PASS] %s:%d - Partial line display\n", __FILE__, __LINE__);
    } else {
        g_test_stats.failed_tests++;
        printf("[FAIL] %s:%d - Expected partial line format not found\n"
               "Expected hex: %s\nExpected ASCII: %s\nGot: %s\n", 
               __FILE__, __LINE__, expected_hex, expected_ascii, captured_output);
    }
    g_test_stats.total_tests++;
}

// 测试多行输出
TEST_CASE(multiple_lines) {
    reset_capture();
    
    // 创建一个32字节的数据块（应该产生2行输出）
    uint8_t test_data[32];
    for (int i = 0; i < 32; i++) {
        test_data[i] = i + 65; // 从'A'开始
    }
    
    // 执行hexdump
    hexdump(test_data, sizeof(test_data), capture_output);
    
    // 验证输出包含两行，且地址正确递增
    const char* first_line_addr = "00000000:";
    const char* second_line_addr = "00000010:";
    
    bool contains_first = (strstr(captured_output, first_line_addr) != nullptr);
    bool contains_second = (strstr(captured_output, second_line_addr) != nullptr);
    
    if (contains_first && contains_second) {
        g_test_stats.passed_tests++;
        printf("[PASS] %s:%d - Multiple lines with correct addresses\n", __FILE__, __LINE__);
    } else {
        g_test_stats.failed_tests++;
        printf("[FAIL] %s:%d - Expected multiple line addresses not found\n"
               "Expected first: %s\nExpected second: %s\nGot: %s\n", 
               __FILE__, __LINE__, first_line_addr, second_line_addr, captured_output);
    }
    g_test_stats.total_tests++;
}

// 测试空缓冲区
TEST_CASE(empty_buffer) {
    reset_capture();
    
    // 执行hexdump，但大小为0
    uint8_t test_data[1] = {0};
    hexdump(test_data, 0, capture_output);
    
    // 验证没有输出
    if (captured_index == 0) {
        g_test_stats.passed_tests++;
        printf("[PASS] %s:%d - Empty buffer produces no output\n", __FILE__, __LINE__);
    } else {
        g_test_stats.failed_tests++;
        printf("[FAIL] %s:%d - Empty buffer should produce no output, but got: %s\n", 
               __FILE__, __LINE__, captured_output);
    }
    g_test_stats.total_tests++;
}

// 测试大地址值
TEST_CASE(large_address) {
    reset_capture();
    
    // 创建一个数据块
    uint8_t test_data[16];
    for (int i = 0; i < 16; i++) {
        test_data[i] = i;
    }
    
    // 使用自定义地址的hexdump函数
    // 注意：我们不能直接修改hexdump函数的address变量，所以这里我们只能测试格式化函数
    char buffer[20];
    format_string(buffer, sizeof(buffer), "%08zx:", (size_t)0xFFFFFFFF);
    
    // 验证地址格式化正确
    const char* expected = "ffffffff:";
    
    if (strcmp(buffer, expected) == 0) {
        g_test_stats.passed_tests++;
        printf("[PASS] %s:%d - Large address formatting\n", __FILE__, __LINE__);
    } else {
        g_test_stats.failed_tests++;
        printf("[FAIL] %s:%d - Expected large address format not correct\n"
               "Expected: %s\nGot: %s\n", 
               __FILE__, __LINE__, expected, buffer);
    }
    g_test_stats.total_tests++;
}

int main() {
    printf("Running hexdump tests...\n");
    
    RUN_TEST(basic_hexdump);
    RUN_TEST(ascii_display);
    RUN_TEST(nonprintable_chars);
    RUN_TEST(partial_line);
    RUN_TEST(multiple_lines);
    RUN_TEST(empty_buffer);
    RUN_TEST(large_address);
    
    print_test_results();
    
    return g_test_stats.failed_tests > 0 ? 1 : 0;
}