#pragma once

#include <stdint.h>
#include <stddef.h>
#include <lib/string.h>

namespace kernel {

// CPIO格式的魔数 (new ASCII format)
const char CPIO_MAGIC[7] = "070701";

// CPIO文件头结构 (new ASCII format)
struct CPIOHeader {
    char magic[6];          // 魔数，固定为"070701"
    char ino[8];            // inode号
    char mode[8];           // 文件模式
    char uid[8];            // 用户ID
    char gid[8];            // 组ID
    char nlink[8];          // 硬链接数
    char mtime[8];          // 修改时间
    char filesize[8];       // 文件大小
    char devmajor[8];       // 主设备号
    char devminor[8];       // 次设备号
    char rdevmajor[8];      // 特殊文件主设备号
    char rdevminor[8];      // 特殊文件次设备号
    char namesize[8];       // 文件名长度（包含结尾的\0）
    char check[8];          // 校验和（未使用，通常为0）
};

inline bool magic_match(const char* magic) {
    return strncmp(magic, CPIO_MAGIC, 6) == 0;
}

// 从CPIO头部获取文件大小
inline uint32_t get_filesize(const CPIOHeader* header) {
    uint32_t size = 0;
    // 解析8个ASCII十六进制字符为32位整数
    for (int i = 0; i < 8; i++) {
        size <<= 4;
        char c = header->filesize[i];
        if (c >= '0' && c <= '9') {
            size |= (c - '0');
        } else if (c >= 'a' && c <= 'f') {
            size |= (c - 'a' + 10);
        } else if (c >= 'A' && c <= 'F') {
            size |= (c - 'A' + 10);
        }
    }
    return size;
}

// 从CPIO头部获取文件名长度
inline uint16_t get_namesize(const CPIOHeader* header) {
    uint16_t size = 0;
    // 解析8个ASCII十六进制字符为16位整数
    for (int i = 0; i < 8; i++) {
        size <<= 4;
        char c = header->namesize[i];
        if (c >= '0' && c <= '9') {
            size |= (c - '0');
        } else if (c >= 'a' && c <= 'f') {
            size |= (c - 'a' + 10);
        } else if (c >= 'A' && c <= 'F') {
            size |= (c - 'A' + 10);
        }
    }
    return size;
}

// 从CPIO头部获取文件模式
inline uint16_t get_mode(const CPIOHeader* header) {
    uint16_t mode = 0;
    // 解析8个ASCII十六进制字符为16位整数
    for (int i = 0; i < 8; i++) {
        mode <<= 4;
        char c = header->mode[i];
        if (c >= '0' && c <= '9') {
            mode |= (c - '0');
        } else if (c >= 'a' && c <= 'f') {
            mode |= (c - 'a' + 10);
        } else if (c >= 'A' && c <= 'F') {
            mode |= (c - 'A' + 10);
        }
    }
    return mode;
}

// 检查是否为目录
inline bool is_directory(const CPIOHeader* header) {
    return (get_mode(header) & 0xF000) == 0x4000;
}

// 检查是否为普通文件
inline bool is_regular(const CPIOHeader* header) {
    return (get_mode(header) & 0xF000) == 0x8000;
}

// 检查是否为CPIO文件结束标记
inline bool is_trailer(const char* name) {
    return name[0] == 'T' && name[1] == 'R' && name[2] == 'A' &&
           name[3] == 'I' && name[4] == 'L' && name[5] == 'E' &&
           name[6] == 'R' && name[7] == '!' && name[8] == '!';
}

} // namespace kernel