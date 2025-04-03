#pragma once
#include <kernel/vfs.h>
#include <lib/console.h>

namespace kernel
{

class ConsoleDevice : public FileDescriptor
{
public:
    ConsoleDevice();
    virtual ~ConsoleDevice();

    // 键盘输入缓冲区大小
    static const size_t INPUT_BUFFER_SIZE = 1024;

    ssize_t read(void* buffer, size_t size) override;
    ssize_t write(const void* buffer, size_t size) override;
    int seek(size_t offset) override;
    int close() override;
    int iterate(void* buffer, size_t buffer_size, uint32_t* pos) override;

private:
    char input_buffer[INPUT_BUFFER_SIZE];
    size_t read_pos = 0;
    volatile size_t buffer_size = 0;
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

    FileDescriptor* open(const char* path) override
    {
        if (g_console_device == nullptr) {
            g_console_device = new ConsoleDevice();
        }
        return g_console_device;
    }

    virtual int stat(const char* path, FileAttribute* attr) override
    {
        if(attr) {
            attr->type = FT_CHR;
            attr->mode = 0666; // 设置读写权限
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
public:
    static ConsoleDevice * get_console_device() {  return g_console_device;}
private:
    static ConsoleDevice * g_console_device;
};

} // namespace kernel