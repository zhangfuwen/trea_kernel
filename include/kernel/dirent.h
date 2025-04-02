#pragma once

#include <stdint.h>

namespace kernel
{

// 目录项结构，用于getdents系统调用
struct dirent {
    uint32_t d_ino;     // inode号
    uint32_t d_off;     // 在目录文件中的偏移
    uint16_t d_reclen;  // 此记录的长度
    uint8_t  d_type;    // 文件类型
    char     d_name[];  // 文件名，以null结尾
};

// 文件类型定义，与FileType枚举对应
enum {
    DT_UNKNOWN = 0,
    DT_REG     = 1,  // 普通文件
    DT_DIR     = 2,  // 目录
    DT_CHR     = 3,  // 字符设备
    DT_BLK     = 4,  // 块设备
    DT_FIFO    = 5,  // FIFO
    DT_SOCK    = 6,  // 套接字
    DT_LNK     = 7,  // 符号链接
};

} // namespace kernel