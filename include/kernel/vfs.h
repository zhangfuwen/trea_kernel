#pragma once

#include <stddef.h>
#include <cstdint>
#include <unistd.h>
//#include <sys/types.h>

struct ProcessControlBlock;
namespace kernel
{

// 文件类型枚举
enum FileType {
    FT_UNKNOWN = 0,
    FT_REG     = 1,  // 普通文件
    FT_DIR     = 2,  // 目录
    FT_CHR     = 3,  // 字符设备
    FT_BLK     = 4,  // 块设备
    FT_FIFO    = 5,  // FIFO
    FT_SOCK    = 6,  // 套接字
    FT_LNK     = 7,  // 符号链接
};

// 文件属性结构
struct FileAttribute {
    FileType type;    // 文件类型
    size_t size;      // 文件大小
    uint32_t mode;    // 文件权限
    uint32_t uid;     // 用户ID
    uint32_t gid;     // 组ID
    uint32_t atime;   // 访问时间
    uint32_t mtime;   // 修改时间
    uint32_t ctime;   // 创建时间
    uint32_t blksize; // 块大小
    uint32_t blocks;  // 块数量
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
    virtual int iterate(void* buffer, size_t buffer_size, uint32_t* pos) = 0;
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

    // 切换当前工作目录
    int chdir(const char* path);

    // 获取当前工作目录
    const char* getcwd();

private:
    VFSManager() = default;
    char current_working_directory[256];
};

} // namespace kernel