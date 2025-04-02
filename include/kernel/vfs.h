#pragma once

#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

struct ProcessControlBlock;
namespace kernel
{

// 文件类型
enum class FileType : unsigned char {
    Regular,   // 普通文件
    Directory, // 目录
    Device     // 设备文件
};

// 文件权限
struct FilePermission {
    bool read;    // 读权限
    bool write;   // 写权限
    bool execute; // 执行权限
};

// 文件属性
struct FileAttribute {
    FileType type;       // 文件类型
    FilePermission perm; // 文件权限
    size_t size;         // 文件大小
};

int sys_open(uint32_t path_ptr, ProcessControlBlock* pcb);
int sys_read(uint32_t fd_num, uint32_t buffer_ptr, uint32_t size, ProcessControlBlock* pcb);
int sys_write(uint32_t fd_num, uint32_t buffer_ptr, uint32_t size, ProcessControlBlock* pcb);
int sys_close(uint32_t fd_num, ProcessControlBlock* pcb);
int sys_seek(uint32_t fd_num, uint32_t offset, ProcessControlBlock* pcb);

void init_vfs();
// 文件描述符
class FileDescriptor
{
public:
    virtual ~FileDescriptor() = default;
    virtual ssize_t read(void* buffer, size_t size) = 0;
    virtual ssize_t write(const void* buffer, size_t size) = 0;
    virtual int seek(size_t offset) = 0;
    virtual int close() = 0;
};

// 文件系统接口
class FileSystem
{
public:
    virtual void print() {};
    virtual ~FileSystem() = default;

    virtual char* get_name() = 0;

    // 打开文件，返回文件描述符
    virtual FileDescriptor* open(const char* path) = 0;

    // 获取文件属性
    virtual int stat(const char* path, FileAttribute* attr) = 0;

    // 创建目录
    virtual int mkdir(const char* path) = 0;

    // 删除文件
    virtual int unlink(const char* path) = 0;

    // 删除目录
    virtual int rmdir(const char* path) = 0;

    // 列出目录内容
    virtual int list(const char* path, void* buffer, size_t buffer_size) { return -1; }

    // 遍历目录内容，用于getdents系统调用
    virtual int iterate(const char* path, void* buffer, size_t buffer_size, uint32_t* pos) { return -1; }
};

// VFS管理器
class VFSManager
{
public:
    static VFSManager& instance()
    {
        static VFSManager inst;
        return inst;
    }

    // 注册文件系统
    void register_fs(const char* mount_point, FileSystem* fs);

    // 打开文件
    FileDescriptor* open(const char* path);

    // 获取文件属性
    int stat(const char* path, FileAttribute* attr);

    // 创建目录
    int mkdir(const char* path);

    // 删除文件
    int unlink(const char* path);

    // 删除目录
    int rmdir(const char* path);

    // 列出目录内容
    int list(const char* path, void* buffer, size_t buffer_size);

    // 遍历目录内容，用于getdents系统调用
    int iterate(const char* path, void* buffer, size_t buffer_size, uint32_t* pos);

private:
    VFSManager() = default;
};

} // namespace kernel