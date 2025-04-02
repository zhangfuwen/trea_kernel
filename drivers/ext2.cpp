#include "drivers/block_device.h"
#include <drivers/ext2.h>
#include <lib/string.h>

namespace kernel
{

Ext2FileSystem::Ext2FileSystem(BlockDevice* device) : device(device)
{
    super_block = new Ext2SuperBlock();
    if(!read_super_block()) {
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

Ext2FileSystem::~Ext2FileSystem()
{
    if(super_block) {
        delete super_block;
    }
}

char* Ext2FileSystem::get_name()
{
    return "ext2";
}

bool Ext2FileSystem::read_super_block()
{
    if(!device->read_block(0, super_block)) {
        return false;
    }
    return super_block->magic == EXT2_MAGIC;
}

Ext2Inode* Ext2FileSystem::read_inode(uint32_t inode_num)
{
    if(inode_num == 0 || inode_num > super_block->inodes_count) {
        return nullptr;
    }

    // 计算inode所在的块
    uint32_t block_size = device->get_info().block_size;
    uint32_t inodes_per_block = block_size / sizeof(Ext2Inode);
    uint32_t block_num = (inode_num - 1) / inodes_per_block + super_block->first_data_block;
    uint32_t offset = (inode_num - 1) % inodes_per_block;

    // 读取整个块
    auto block_buffer = new uint8_t[block_size];
    if(!device->read_block(block_num, block_buffer)) {
        delete[] block_buffer;
        return nullptr;
    }

    // 复制inode数据
    auto inode = new Ext2Inode();
    memcpy(inode, block_buffer + offset * sizeof(Ext2Inode), sizeof(Ext2Inode));
    delete[] block_buffer;

    return inode;
}

bool Ext2FileSystem::write_inode(uint32_t inode_num, const Ext2Inode* inode)
{
    if(inode_num == 0 || inode_num > super_block->inodes_count || !inode) {
        return false;
    }

    // 计算inode所在的块
    uint32_t block_size = device->get_info().block_size;
    uint32_t inodes_per_block = block_size / sizeof(Ext2Inode);
    uint32_t block_num = (inode_num - 1) / inodes_per_block + super_block->first_data_block;
    uint32_t offset = (inode_num - 1) % inodes_per_block;

    // 读取整个块
    auto block_buffer = new uint8_t[block_size];
    if(!device->read_block(block_num, block_buffer)) {
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

uint32_t Ext2FileSystem::allocate_block()
{
    // 简单的块分配策略：从头开始查找第一个空闲块
    auto block_buffer = new uint8_t[device->get_info().block_size];
    for(uint32_t i = super_block->first_data_block; i < super_block->blocks_count; i++) {
        if(device->read_block(i, block_buffer)) {
            bool is_free = true;
            for(size_t j = 0; j < device->get_info().block_size; j++) {
                if(block_buffer[j] != 0) {
                    is_free = false;
                    break;
                }
            }
            if(is_free) {
                delete[] block_buffer;
                return i;
            }
        }
    }
    delete[] block_buffer;
    return 0;
}

uint32_t Ext2FileSystem::allocate_inode()
{
    // 简单的inode分配策略：返回第一个可用的inode号
    for(uint32_t i = 1; i <= super_block->inodes_count; i++) {
        auto inode = read_inode(i);
        if(inode && inode->mode == 0) {
            delete inode;
            return i;
        }
        delete inode;
    }
    return 0;
}

// 块/节点释放函数
void free_block(uint32_t block_num)
{
    // 实现块释放逻辑
}

void free_inode(uint32_t inode_num)
{
    // 实现inode释放逻辑
}

FileDescriptor* Ext2FileSystem::open(const char* path)
{

    // 从根目录开始查找（inode 2）
    uint32_t current_inode = 2;

    // 分割路径
    char* parts = strdup(path);
    char* token = strtok(parts, "/");

    while(token) {
        Ext2Inode* inode = read_inode(current_inode);
        if(!inode || (inode->mode & 0xF000) != 0x4000) {
            delete inode;
            delete[] parts;
            return nullptr; // 不是目录
        }

        // 读取目录块
        auto block_size = super_block->block_size;
        uint8_t* block = new uint8_t[block_size];
        bool found = false;
        for(uint32_t i = 0; i < inode->blocks; i++) {
            device->read_block(inode->i_block[i], block);

            Ext2DirEntry* entry = (Ext2DirEntry*)block;
            while((uint8_t*)entry < block + block_size) {
                if(entry->inode && entry->name_len == strlen(token) &&
                    memcmp(entry->name, token, entry->name_len) == 0) {
                    current_inode = entry->inode;
                    if(entry->file_type == (unsigned char)FileType::Directory) {
                        current_dir_inode = current_inode;
                    }
                    found = true;
                    break;
                }
                entry = (Ext2DirEntry*)((uint8_t*)entry + entry->rec_len);
            }
            if(found)
                break;
        }

        delete[] block;
        delete inode;
        if(!found) {
            delete[] parts;
            return nullptr;
        }
        token = strtok(nullptr, "/");
    }

    delete[] parts;
    return new Ext2FileDescriptor(current_inode, this);
}

int Ext2FileSystem::stat(const char* path, FileAttribute* attr)
{
    Ext2FileDescriptor* fd = (Ext2FileDescriptor*)open(path);
    if(!fd)
        return -1;

    Ext2Inode* inode = read_inode(fd->m_inode);
    attr->size = inode->size;
    attr->type = (inode->mode & 0xF000) == 0x4000 ? FileType::Directory : FileType::Regular;
    delete inode;
    delete fd;
    return 0;
}

// 修改目录创建代码中的父目录引用
int Ext2FileSystem::mkdir(const char* path)
{
    // 创建目录inode
    uint32_t new_inode = allocate_inode();
    if(!new_inode)
        return -1;

    // 初始化目录inode
    auto block_size = super_block->block_size;
    Ext2Inode dir_inode{};
    dir_inode.mode = 0x41FF; // 目录权限
    dir_inode.size = block_size;
    dir_inode.blocks = 1;
    dir_inode.i_block[0] = allocate_block(); // 使用正确的块指针字段

    write_inode(new_inode, &dir_inode);

    // 创建目录条目
    uint8_t* block = new uint8_t[block_size];
    Ext2DirEntry* self = (Ext2DirEntry*)block;
    self->inode = new_inode;
    self->name_len = 1;
    self->rec_len = 12;
    memcpy(self->name, ".", 1);

    Ext2DirEntry* parent = (Ext2DirEntry*)(block + 12);
    parent->inode = current_dir_inode; // 使用当前目录上下文
    parent->name_len = 2;
    parent->rec_len = block_size - 12;
    memcpy(parent->name, "..", 2);

    device->write_block(dir_inode.i_block[0], block); // 修改此处
    delete[] block;

    // 添加条目到父目录
    // ... 需要实现目录条目添加逻辑 ...
    return 0;
}

int Ext2FileSystem::unlink(const char* path)
{
    // TODO: 实现文件删除
    Ext2FileDescriptor* fd = (Ext2FileDescriptor*)open(path);
    if(!fd)
        return -1;

    Ext2Inode* inode = read_inode(fd->m_inode);
    inode->i_links_count--; // 修正字段名
    write_inode(fd->m_inode, inode);

    if(inode->i_links_count == 0) {
        // 释放数据块
        for(uint32_t i = 0; i < inode->i_blocks; i++) { // 修正字段名
            free_block(inode->i_block[i]);              // 修正字段名
        }
        free_inode(fd->m_inode);
    }

    delete inode;
    delete fd;
    return 0;
}

int Ext2FileSystem::rmdir(const char* path)
{
    // TODO: 实现目录删除
    // 需要额外检查目录是否为空
    return unlink(path); // 暂时共用unlink逻辑
}

// 实现文件描述符方法
Ext2FileDescriptor::Ext2FileDescriptor(uint32_t inode, Ext2FileSystem* fs)
    : m_inode(inode), m_position(0), m_fs(fs)
{
}

 Ext2FileDescriptor::~Ext2FileDescriptor()
{

}


int Ext2FileDescriptor::seek(size_t offset)
{
    Ext2Inode* inode = m_fs->read_inode(m_inode);
    if(!inode)
        return 0;
    off_t new_pos = m_position;
    auto whence = SEEK_SET;
    switch(whence) {
        case SEEK_SET:
            new_pos = offset;
            break;
        case SEEK_CUR:
            new_pos += offset;
            break;
        case SEEK_END:
            new_pos = inode->size + offset;
            break;
        default:
            delete inode;
            return -1;
    }

    if(new_pos < 0 || new_pos > inode->size) {
        delete inode;
        return -1;
    }

    m_position = new_pos;
    delete inode;
    return m_position;
}

int Ext2FileDescriptor::close()
{
    // 释放相关资源
    // return m_fs->close(this);
    return 0;
}

ssize_t Ext2FileDescriptor::read(void* buffer, size_t size)
{
    Ext2Inode* inode = m_fs->read_inode(m_inode);
    if(!inode)
        return -1;

    uint32_t block_size = m_fs->device->get_info().block_size;
    size_t bytes_remaining = inode->size - m_position;
    size_t bytes_to_read = min(size, bytes_remaining);
    size_t total_read = 0;

    while(bytes_to_read > 0) {
        // 计算当前块号
        uint32_t block_idx = m_position / block_size;
        uint32_t block_offset = m_position % block_size;
        uint32_t block_num = inode->i_block[block_idx];

        // 读取整个块
        uint8_t* block = new uint8_t[block_size];
        if(!m_fs->device->read_block(block_num, block)) {
            delete[] block;
            break;
        }

        // 复制数据到缓冲区
        size_t copy_size = min(block_size - block_offset, bytes_to_read);
        memcpy(static_cast<uint8_t*>(buffer) + total_read, block + block_offset, copy_size);

        m_position += copy_size;
        total_read += copy_size;
        bytes_to_read -= copy_size;
        delete[] block;
    }

    delete inode;
    return total_read;
}

// 在Ext2FileSystem类实现之外添加
ssize_t Ext2FileDescriptor::write(const void* buffer, size_t size) {
    Ext2Inode* inode = m_fs->read_inode(m_inode);
    if (!inode || (inode->mode & 0xF000) == 0x4000) { // 目录不可写
        delete inode;
        return 0;
    }

    uint32_t block_size = m_fs->device->get_info().block_size;
    const uint8_t* src = static_cast<const uint8_t*>(buffer);
    size_t total_written = 0;

    while (total_written < size) {
        // 计算当前块索引和偏移
        uint32_t block_idx = m_position / block_size;
        uint32_t block_offset = m_position % block_size;

        // 需要分配新块的情况
        if (block_idx >= inode->i_blocks) {
            uint32_t new_block = m_fs->allocate_block();
            if (!new_block) break;

            inode->i_block[block_idx] = new_block;
            inode->i_blocks++;
        }

        // 读取现有块数据（如果需要部分写入）
        uint8_t* block_data = new uint8_t[block_size];
        if (block_offset > 0 || (size - total_written) < block_size) {
            m_fs->device->read_block(inode->i_block[block_idx], block_data);
        }

        // 计算本次写入量
        size_t write_size = min(size - total_written, block_size - block_offset);
        memcpy(block_data + block_offset, src + total_written, write_size);

        // 写入块设备
        if (!m_fs->device->write_block(inode->i_block[block_idx], block_data)) {
            delete[] block_data;
            break;
        }

        // 更新状态
        total_written += write_size;
        m_position += write_size;
        delete[] block_data;

        // 扩展文件大小
        if (m_position > inode->size) {
            inode->size = m_position;
        }
    }

    // 更新inode信息
    // inode->mtime = time(nullptr);
    m_fs->write_inode(m_inode, inode);
    delete inode;

    return total_written;
}

} // namespace kernel

// 其他接口方法实现...} // namespace kernel