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

// 查找字符串中最后一个匹配的字符
char* strrchr(const char* s, int c);

// 内存复制
void* memcpy(void* dest, const void* src, size_t n);

// 内存设置
void* memset(void* s, int c, size_t n);