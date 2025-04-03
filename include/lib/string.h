#pragma once
#include <stddef.h>

// 字符串长度计算
size_t strlen(const char* str);

// 字符串复制
char* strcpy(char* dest, const char* src);

// 字符串比较
int strcmp(const char* s1, const char* s2);

// 字符串部分复制
char* strncpy(char* dest, const char* src, size_t n);

// 字符串部分比较
int strncmp(const char* s1, const char* s2, size_t n);

// 查找字符串中的字符
char* strchr(const char* s, int c);
char* strstr(const char* haystack, const char* needle);

// 查找字符串中最后一个匹配的字符
char* strrchr(const char* s, int c);

// 内存复制
void* memcpy(void* dest, const void* src, size_t n);

// 内存设置
void memset(void* s, int c, size_t n);

// 添加以下声明
int memcmp(const void* s1, const void* s2, size_t n);
char* strdup(const char* s);
char* strtok(char* str, const char* delim);
char* strtok(char* str, const char* delim);

// 添加在文件末尾
template<typename T>
const T& min(const T& a, const T& b) {
    return (a < b) ? a : b;
}

template<typename T>
const T& max(const T& a, const T& b) {
    return (a > b) ? a : b;
}

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))