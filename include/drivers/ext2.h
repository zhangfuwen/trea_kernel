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
    uint8_t padding[512-30];
    void print();
};

// Inode结构
struct Ext2Inode {
    uint16_t mode;                  // 文件类型和权限
    uint32_t size;                  // 文件大小
    uint32_t blocks;                // 占用的块数
    uint32_t direct_blocks[EXT2_DIRECT_BLOCKS];  // 直接块
    uint32_t indirect_block;        // 一级间接块
    uint32_t double_indirect_block; // 二级间接块
    uint16_t i_links_count;  // 链接计数
    uint16_t i_blocks;       // 使用块数
    uint32_t i_block[15];    // 数据块指针
};

// 目录项结构
struct Ext2DirEntry {
    uint32_t inode;                 // Inode号
    uint16_t rec_len;               // 目录项长度
    uint8_t  name_len;              // 名称长度
    uint8_t  file_type;             // 文件类型
    char     name[EXT2_NAME_LEN];   // 文件名
};

class Ext2FileDescriptor;
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
    virtual int list(const char* path, void* buffer, size_t buffer_size) override;
    virtual int iterate(const char* path, void* buffer, size_t buffer_size, uint32_t* pos) override;

private:
    friend class Ext2FileDescriptor;
    BlockDevice* device;
    Ext2SuperBlock* super_block;
    uint32_t current_dir_inode = 2; // 默认根目录

    // 内部辅助函数
    bool read_super_block();
    Ext2Inode* read_inode(uint32_t inode_num);
    bool write_inode(uint32_t inode_num, const Ext2Inode* inode);
    uint32_t allocate_block();
    uint32_t allocate_inode();
};

class Ext2FileDescriptor : public kernel::FileDescriptor {
public:
    Ext2FileDescriptor(uint32_t inode, Ext2FileSystem* fs);
    ~Ext2FileDescriptor() override;

    ssize_t read(void* buffer, size_t size) override;
    ssize_t write(const void* buffer, size_t size) override;
    int seek(size_t offset) override;
    int close() override;

private:
    friend class Ext2FileSystem;
    uint32_t m_inode;
    off_t m_position;
    Ext2FileSystem* m_fs;
};

} // namespace kernel
