#include "ext2.h"
#include <lib/string.h>

namespace kernel {

Ext2FileSystem::Ext2FileSystem(BlockDevice* device) : device(device) {
    super_block = new Ext2SuperBlock();
    if (!read_super_block()) {
        // 初始化新的文件系统
        super_block->inodes_count = 1024;
        super_block->blocks_count = device->get_info().total_blocks;
        super_block->first_data_block = 1;
        super_block->block_size = device->get_info().block_size;
        super_block->blocks_per_group = 8192;
        super_block->inodes_per_group = 1024;
        super_block->magic = EXT2_MAGIC;
        super_block->state = 1; // 文件系统正常

        // 写入超级块
        device->write_block(0, super_block);

        // 初始化根目录
        auto root_inode = new Ext2Inode();
        root_inode->mode = 0x4000; // 目录类型
        root_inode->size = 0;
        root_inode->blocks = 0;
        write_inode(1, root_inode);
        delete root_inode;
    }
}

Ext2FileSystem::~Ext2FileSystem() {
    if (super_block) {
        delete super_block;
    }
}

char* Ext2FileSystem::get_name() {
    return "ext2";
}

bool Ext2FileSystem::read_super_block() {
    if (!device->read_block(0, super_block)) {
        return false;
    }
    return super_block->magic == EXT2_MAGIC;
}

Ext2Inode* Ext2FileSystem::read_inode(uint32_t inode_num) {
    if (inode_num == 0 || inode_num > super_block->inodes_count) {
        return nullptr;
    }

    // 计算inode所在的块
    uint32_t block_size = device->get_info().block_size;
    uint32_t inodes_per_block = block_size / sizeof(Ext2Inode);
    uint32_t block_num = (inode_num - 1) / inodes_per_block + super_block->first_data_block;
    uint32_t offset = (inode_num - 1) % inodes_per_block;

    // 读取整个块
    auto block_buffer = new uint8_t[block_size];
    if (!device->read_block(block_num, block_buffer)) {
        delete[] block_buffer;
        return nullptr;
    }

    // 复制inode数据
    auto inode = new Ext2Inode();
    memcpy(inode, block_buffer + offset * sizeof(Ext2Inode), sizeof(Ext2Inode));
    delete[] block_buffer;

    return inode;
}

bool Ext2FileSystem::write_inode(uint32_t inode_num, const Ext2Inode* inode) {
    if (inode_num == 0 || inode_num > super_block->inodes_count || !inode) {
        return false;
    }

    // 计算inode所在的块
    uint32_t block_size = device->get_info().block_size;
    uint32_t inodes_per_block = block_size / sizeof(Ext2Inode);
    uint32_t block_num = (inode_num - 1) / inodes_per_block + super_block->first_data_block;
    uint32_t offset = (inode_num - 1) % inodes_per_block;

    // 读取整个块
    auto block_buffer = new uint8_t[block_size];
    if (!device->read_block(block_num, block_buffer)) {
        delete[] block_buffer;
        return false;
    }

    // 更新inode数据
    memcpy(block_buffer + offset * sizeof(Ext2Inode), inode, sizeof(Ext2Inode));

    // 写回块
    bool result = device->write_block(block_num, block_buffer);
    delete[] block_buffer;

    return result;
}

uint32_t Ext2FileSystem::allocate_block() {
    // 简单的块分配策略：从头开始查找第一个空闲块
    auto block_buffer = new uint8_t[device->get_info().block_size];
    for (uint32_t i = super_block->first_data_block; i < super_block->blocks_count; i++) {
        if (device->read_block(i, block_buffer)) {
            bool is_free = true;
            for (size_t j = 0; j < device->get_info().block_size; j++) {
                if (block_buffer[j] != 0) {
                    is_free = false;
                    break;
                }
            }
            if (is_free) {
                delete[] block_buffer;
                return i;
            }
        }
    }
    delete[] block_buffer;
    return 0;
}

uint32_t Ext2FileSystem::allocate_inode() {
    // 简单的inode分配策略：返回第一个可用的inode号
    for (uint32_t i = 1; i <= super_block->inodes_count; i++) {
        auto inode = read_inode(i);
        if (inode && inode->mode == 0) {
            delete inode;
            return i;
        }
        delete inode;
    }
    return 0;
}

FileDescriptor* Ext2FileSystem::open(const char* path) {
    // TODO: 实现文件打开操作
    return nullptr;
}

int Ext2FileSystem::stat(const char* path, FileAttribute* attr) {
    // TODO: 实现文件状态查询
    return -1;
}

int Ext2FileSystem::mkdir(const char* path) {
    // TODO: 实现目录创建
    return -1;
}

int Ext2FileSystem::unlink(const char* path) {
    // TODO: 实现文件删除
    return -1;
}

int Ext2FileSystem::rmdir(const char* path) {
    // TODO: 实现目录删除
    return -1;
}

} // namespace kernel