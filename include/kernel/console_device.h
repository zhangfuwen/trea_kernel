#pragma once
#include <kernel/vfs.h>
#include <lib/console.h>

#include <termios.h>

//
// struct termios {
//     tcflag_t c_iflag;  // 输入模式标志
//     tcflag_t c_oflag;  // 输出模式标志
//     tcflag_t c_cflag;  // 控制模式标志
//     tcflag_t c_lflag;  // 本地模式标志
//     cc_t c_cc[NCCS];   // 控制字符
// };

// 终端控制函数声明
int tcgetattr(int fd, struct termios *termios_p);
int tcsetattr(int fd, int optional_actions, const struct termios *termios_p);
int tcdrain(int fd);
int tcflow(int fd, int action);
int tcflush(int fd, int queue_selector);
int tcsendbreak(int fd, int duration);



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

    const char* get_name() override
    {
        return "consolefs";
    }

    FileDescriptor* open([[maybe_unused]] const char* path) override
    {
        return &g_console_device;
    }

    virtual int stat([[maybe_unused]] const char* path, FileAttribute* attr) override
    {
        if(attr) {
            attr->type = FT_CHR;
            attr->mode = 0666; // 设置读写权限
            attr->size = 0;
        }
        return 0;
    }

    virtual int mkdir([[maybe_unused]] const char* path) override
    {
        return -1;
    }
    virtual int unlink([[maybe_unused]] const char* path) override
    {
        return -1;
    }
    virtual int rmdir([[maybe_unused]] const char* path) override
    {
        return -1;
    }
public:
    static ConsoleDevice * get_console_device() {  return &g_console_device;}
private:
    static ConsoleDevice g_console_device;
};

} // namespace kernel