#ifndef CUSTOM_KERNEL_TEST_FRAMEWORK_H
#define CUSTOM_KERNEL_TEST_FRAMEWORK_H

#include <cstdio>
// #include <cstring>

// 测试结果统计
struct TestStats {
    int total_tests = 0;
    int passed_tests = 0;
    int failed_tests = 0;
};

static TestStats g_test_stats;

// 断言宏
#define ASSERT_EQ(expected, actual) \
    do { \
        g_test_stats.total_tests++; \
        if ((expected) == (actual)) { \
            g_test_stats.passed_tests++; \
            printf("[PASS] %s:%d - Assert Equal\n", __FILE__, __LINE__); \
        } else { \
            g_test_stats.failed_tests++; \
            printf("[FAIL] %s:%d - Expected %d, got %d\n", __FILE__, __LINE__, (expected), (actual)); \
        } \
    } while(0)

#define ASSERT_STR_EQ(expected, actual) \
    do { \
        g_test_stats.total_tests++; \
        if (strcmp((expected), (actual)) == 0) { \
            g_test_stats.passed_tests++; \
            printf("[PASS] %s:%d - Assert String Equal\n", __FILE__, __LINE__); \
        } else { \
            g_test_stats.failed_tests++; \
            printf("[FAIL] %s:%d - Expected \"%s\", got \"%s\"\n", __FILE__, __LINE__, (expected), (actual)); \
        } \
    } while(0)

// 测试用例宏
#define TEST_CASE(name) \
    void test_##name()

// 运行测试用例宏
#define RUN_TEST(name) \
    do { \
        printf("\nRunning test case: %s\n", #name); \
        test_##name(); \
    } while(0)

// 打印测试结果
inline void print_test_results() {
    printf("\n=== Test Results ===\n");
    printf("Total tests: %d\n", g_test_stats.total_tests);
    printf("Passed: %d\n", g_test_stats.passed_tests);
    printf("Failed: %d\n", g_test_stats.failed_tests);
}

#endif // CUSTOM_KERNEL_TEST_FRAMEWORK_H