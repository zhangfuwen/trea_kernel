#pragma once
#include <kernel/vfs.h>
#include <lib/console.h>

namespace kernel
{

class ConsoleDevice : public FileDescriptor
{
public:
    ConsoleDevice() = default;
    virtual ~ConsoleDevice() = default;

    virtual ssize_t read(void* buffer, size_t size) override
    {
        // TODO: 实现键盘输入缓冲区
        return 0;
    }

    virtual ssize_t write(const void* buffer, size_t size) override
    {
        const char* chars = static_cast<const char*>(buffer);
        for(size_t i = 0; i < size; i++) {
            Console::putchar(chars[i]);
        }
        return size;
    }

    virtual int seek(size_t offset) override
    {
        return -1; // 控制台设备不支持seek操作
    }

    virtual int close() override
    {
        return 0; // 标准输入输出流不应该被关闭
    }
};

class ConsoleFS : public FileSystem
{
public:
    ConsoleFS() = default;
    virtual ~ConsoleFS() = default;

    char* get_name() override
    {
        return "consolefs";
    }

    virtual FileDescriptor* open(const char* path) override
    {
        return new ConsoleDevice();
    }

    virtual int stat(const char* path, FileAttribute* attr) override
    {
        if(attr) {
            attr->type = FileType::Device;
            attr->perm = {true, true, false};
            attr->size = 0;
        }
        return 0;
    }

    virtual int mkdir(const char* path) override
    {
        return -1;
    }
    virtual int unlink(const char* path) override
    {
        return -1;
    }
    virtual int rmdir(const char* path) override
    {
        return -1;
    }
};

} // namespace kernel