#include "lib/debug.h"
#include "lib/string.h"

// 测试用例结构体
struct TestCase {
    const char* name;
    const char* format;
    const char* expected;
    void (*setup)(char* buffer, size_t size);
};

// 测试辅助函数
static void test_hex_basic(char* buffer, size_t size) {
    format_string(buffer, size, "%x", 255);
}

static void test_hex_width(char* buffer, size_t size) {
    format_string(buffer, size, "%4x", 255);
}

static void test_hex_zero_pad(char* buffer, size_t size) {
    format_string(buffer, size, "%04x", 255);
}

static void test_hex_left_align(char* buffer, size_t size) {
    format_string(buffer, size, "%-4x", 255);
}

static void test_hex_alt_form(char* buffer, size_t size) {
    format_string(buffer, size, "%#x", 255);
}

static void test_hex_large_number(char* buffer, size_t size) {
    format_string(buffer, size, "%08x", 0x12345678);
}

static void test_hex_zero(char* buffer, size_t size) {
    format_string(buffer, size, "%02x", 0);
}

static void test_hex_small_number(char* buffer, size_t size) {
    format_string(buffer, size, "%02x", 15);
}

// 测试用例数组
static const TestCase test_cases[] = {
    {"基本十六进制", "%x", "ff", test_hex_basic},
    {"指定宽度", "%4x", "  ff", test_hex_width},
    {"零填充", "%04x", "00ff", test_hex_zero_pad},
    {"左对齐", "%-4x", "ff  ", test_hex_left_align},
    {"替代形式", "%#x", "0xff", test_hex_alt_form},
    {"大数字", "%08x", "12345678", test_hex_large_number},
    {"零值", "%02x", "00", test_hex_zero},
    {"小数字", "%02x", "0f", test_hex_small_number},
};

// 运行所有测试用例
void run_format_string_tests() {
    char buffer[256];
    int passed = 0;
    int total = sizeof(test_cases) / sizeof(test_cases[0]);

    debug_info("开始运行format_string测试用例...\n");

    for (size_t i = 0; i < total; i++) {
        const TestCase& test = test_cases[i];
        memset(buffer, 0, sizeof(buffer));
        
        test.setup(buffer, sizeof(buffer));
        
        bool test_passed = strcmp(buffer, test.expected) == 0;
        if (test_passed) {
            passed++;
            debug_info("测试 '%s' 通过 (格式: '%s', 结果: '%s')\n",
                      test.name, test.format, buffer);
        } else {
            debug_err("测试 '%s' 失败 (格式: '%s', 期望: '%s', 实际: '%s')\n",
                     test.name, test.format, test.expected, buffer);
        }
    }

    debug_info("测试完成: %d/%d 通过\n", passed, total);
}