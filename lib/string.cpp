#include <stddef.h>

#include "lib/debug.h"

// 字符串长度计算
size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len])
        len++;
    if (len >= 0x1000000) {
        return 0;
    }
    return len;
}

// 字符串复制
char* strcpy(char* dest, const char* src) {
    char* d = dest;
    while ((*d++ = *src++) != '\0');
    return dest;
}

// 字符串部分复制
char* strncpy(char* dest, const char* src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i] != '\0'; i++)
        dest[i] = src[i];
    for (; i < n; i++)
        dest[i] = '\0';
    return dest;
}

// 字符串比较
int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

// 字符串部分比较
int strncmp(const char* s1, const char* s2, size_t n_) {
    int n = n_;
    while (n-- && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    //debug_debug("n: %d, s1 %c, s2 %c\n", n, *s1, *s2);
    return n < 0 ? 0 : *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

// 查找字符串中的字符
char* strchr(const char* s, int c) {
    while (*s != (char)c) {
        if (*s++ == '\0')
            return nullptr;
    }
    return (char*)s;
}

// 查找字符串中最后一个匹配的字符
char* strrchr(const char* s, int c) {
    const char* found = nullptr;
    while (*s) {
        if (*s == (char)c)
            found = s;
        s++;
    }
    if ((char)c == '\0')
        return (char*)s;
    return (char*)found;
}

// 内存复制
void* memcpy(void* dest, const void* src, size_t n) {
    char* d = (char*)dest;
    const char* s = (const char*)src;
    while (n--)
        *d++ = *s++;
    return dest;
}

// 内存设置
void* memset(void* s, int c, size_t n) {
    unsigned char* p = (unsigned char*)s;
    while (n--)
        *p++ = (unsigned char)c;
    return s;
}