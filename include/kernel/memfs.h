#pragma once

#include <kernel/vfs.h>
#include <stddef.h>
#include <stdint.h>

namespace kernel
{

// 内存文件系统的文件节点
struct MemFSInode {
    char name[256];       // 文件名
    FileType type;        // 文件类型
    uint32_t mode;        // 文件权限
    uint8_t* data;        // 文件数据
    size_t size;          // 文件大小
    size_t capacity;      // 数据缓冲区容量
    MemFSInode* parent;   // 父目录
    MemFSInode* children; // 子文件/目录列表
    MemFSInode* next;     // 同级节点链表
    void print();
};

// 内存文件系统的文件描述符
class MemFSFileDescriptor : public FileDescriptor
{
public:
    MemFSFileDescriptor(MemFSInode* inode);
    virtual ~MemFSFileDescriptor();

    virtual ssize_t read(void* buffer, size_t size) override;
    virtual ssize_t write(const void* buffer, size_t size) override;
    virtual int seek(size_t offset) override;
    virtual int close() override;
    virtual int iterate(void* buffer, size_t buffer_size, uint32_t* pos) override;

private:
    MemFSInode* inode;
    size_t offset;
};

// 内存文件系统
class MemFS : public FileSystem
{
public:
    MemFS();
    virtual ~MemFS();

    // 初始化内存文件系统
    void init();

    char* get_name() override;
    void print() override;

    // 加载initramfs数据
    int load_initramfs(const void* data, size_t size);

    // 实现FileSystem接口
    virtual FileDescriptor* open(const char* path) override;
    virtual int stat(const char* path, FileAttribute* attr) override;
    virtual int mkdir(const char* path) override;
    virtual int unlink(const char* path) override;
    virtual int rmdir(const char* path) override;

private:
    MemFSInode* root; // 根目录节点

    // 查找文件节点
    MemFSInode* find_inode(const char* path);

    // 创建新节点
    MemFSInode* create_inode(const char* name, FileType type);

    // 释放节点及其所有子节点
    void free_inode(MemFSInode* inode);
};

} // namespace kernel