#pragma once
#include <stddef.h>
#include <stdint.h>
#include "block_device.h"
#include <kernel/vfs.h>

namespace kernel {

// EXT2文件系统常量
constexpr uint16_t EXT2_MAGIC = 0xEF53;
constexpr uint32_t EXT2_DIRECT_BLOCKS = 12;
constexpr uint32_t EXT2_NAME_LEN = 255;

// 超级块结构
struct Ext2SuperBlock {
    uint32_t inodes_count;          // Inodes数量
    uint32_t blocks_count;          // 块数量
    uint32_t first_data_block;      // 第一个数据块
    uint32_t block_size;            // 块大小
    uint32_t blocks_per_group;      // 每组块数
    uint32_t inodes_per_group;      // 每组inode数
    uint16_t magic;                 // 魔数
    uint16_t state;                 // 文件系统状态
};

// Inode结构
struct Ext2Inode {
    uint16_t mode;                  // 文件类型和权限
    uint32_t size;                  // 文件大小
    uint32_t blocks;                // 占用的块数
    uint32_t direct_blocks[EXT2_DIRECT_BLOCKS];  // 直接块
    uint32_t indirect_block;        // 一级间接块
    uint32_t double_indirect_block; // 二级间接块
};

// 目录项结构
struct Ext2DirEntry {
    uint32_t inode;                 // Inode号
    uint16_t rec_len;               // 目录项长度
    uint8_t  name_len;              // 名称长度
    uint8_t  file_type;             // 文件类型
    char     name[EXT2_NAME_LEN];   // 文件名
};

// EXT2文件系统实现
class Ext2FileSystem : public FileSystem {
public:
    Ext2FileSystem(BlockDevice* device);
    virtual ~Ext2FileSystem();

    // 实现VFS接口
    virtual char* get_name() override;
    virtual FileDescriptor* open(const char* path) override;
    virtual int stat(const char* path, FileAttribute* attr) override;
    virtual int mkdir(const char* path) override;
    virtual int unlink(const char* path) override;
    virtual int rmdir(const char* path) override;

private:
    BlockDevice* device;
    Ext2SuperBlock* super_block;

    // 内部辅助函数
    bool read_super_block();
    Ext2Inode* read_inode(uint32_t inode_num);
    bool write_inode(uint32_t inode_num, const Ext2Inode* inode);
    uint32_t allocate_block();
    uint32_t allocate_inode();
};

} // namespace kernel