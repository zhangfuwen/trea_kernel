#pragma once

#include <stdint.h>
#include <stddef.h>

namespace kernel {

// CPIO格式的魔数
const uint16_t CPIO_MAGIC = 0x71C7;

// CPIO文件头结构
struct CPIOHeader {
    uint16_t magic;          // 魔数，固定为0x71C7
    uint16_t dev;            // 设备号
    uint16_t ino;            // inode号
    uint16_t mode;           // 文件模式
    uint16_t uid;            // 用户ID
    uint16_t gid;            // 组ID
    uint16_t nlink;          // 硬链接数
    uint16_t rdev;           // 特殊文件的设备号
    uint16_t mtime[2];       // 修改时间
    uint16_t namesize;       // 文件名长度（包含结尾的\0）
    uint16_t filesize[2];    // 文件大小
};

// 从CPIO头部获取文件大小
inline uint32_t get_filesize(const CPIOHeader* header) {
    return (uint32_t)header->filesize[0] << 16 | header->filesize[1];
}

// 从CPIO头部获取文件名长度
inline uint16_t get_namesize(const CPIOHeader* header) {
    return header->namesize;
}

// 从CPIO头部获取文件模式
inline uint16_t get_mode(const CPIOHeader* header) {
    return header->mode;
}

// 检查是否为目录
inline bool is_directory(const CPIOHeader* header) {
    return (header->mode & 0xF000) == 0x4000;
}

// 检查是否为普通文件
inline bool is_regular(const CPIOHeader* header) {
    return (header->mode & 0xF000) == 0x8000;
}

// 检查是否为CPIO文件结束标记
inline bool is_trailer(const char* name) {
    return name[0] == 'T' && name[1] == 'R' && name[2] == 'A' &&
           name[3] == 'I' && name[4] == 'L' && name[5] == 'E' &&
           name[6] == 'R' && name[7] == '!' && name[8] == '!';
}

} // namespace kernel