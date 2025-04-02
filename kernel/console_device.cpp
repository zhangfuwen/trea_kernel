#include "kernel/vfs.h"
#include "lib/console.h"
#include "kernel/console_device.h"
#include <drivers/keyboard.h>
#include <arch/x86/interrupt.h>

namespace kernel
{

// 全局控制台设备实例，用于键盘中断处理程序访问
static ConsoleDevice* g_console_device = nullptr;

ConsoleDevice::ConsoleDevice()
{
    // 注册键盘中断处理程序
    g_console_device = this;
    InterruptManager::registerHandler(IRQ_KEYBOARD, []() {
        if (g_console_device) {
            uint8_t scancode = keyboard_get_scancode();
            // 将扫描码转换为ASCII码
            if (scancode < 0x80) { // 只处理按下事件
                // 简单的扫描码到ASCII码的映射
                char ascii = 0;
                switch(scancode) {
                    case 0x1E: ascii = 'a'; break;
                    case 0x30: ascii = 'b'; break;
                    case 0x2E: ascii = 'c'; break;
                    case 0x20: ascii = 'd'; break;
                    case 0x12: ascii = 'e'; break;
                    case 0x21: ascii = 'f'; break;
                    case 0x22: ascii = 'g'; break;
                    case 0x23: ascii = 'h'; break;
                    case 0x17: ascii = 'i'; break;
                    case 0x24: ascii = 'j'; break;
                    case 0x25: ascii = 'k'; break;
                    case 0x26: ascii = 'l'; break;
                    case 0x32: ascii = 'm'; break;
                    case 0x31: ascii = 'n'; break;
                    case 0x18: ascii = 'o'; break;
                    case 0x19: ascii = 'p'; break;
                    case 0x10: ascii = 'q'; break;
                    case 0x13: ascii = 'r'; break;
                    case 0x1F: ascii = 's'; break;
                    case 0x14: ascii = 't'; break;
                    case 0x16: ascii = 'u'; break;
                    case 0x2F: ascii = 'v'; break;
                    case 0x11: ascii = 'w'; break;
                    case 0x2D: ascii = 'x'; break;
                    case 0x15: ascii = 'y'; break;
                    case 0x2C: ascii = 'z'; break;
                    case 0x39: ascii = ' '; break; // 空格
                    case 0x1C: ascii = '\n'; break; // 回车
                    // 数字键0-9
                    case 0x0B: ascii = '0'; break;
                    case 0x02: ascii = '1'; break;
                    case 0x03: ascii = '2'; break;
                    case 0x04: ascii = '3'; break;
                    case 0x05: ascii = '4'; break;
                    case 0x06: ascii = '5'; break;
                    case 0x07: ascii = '6'; break;
                    case 0x08: ascii = '7'; break;
                    case 0x09: ascii = '8'; break;
                    case 0x0A: ascii = '9'; break;
                }
                
                if (ascii != 0 && g_console_device->buffer_size < INPUT_BUFFER_SIZE) {
                    size_t write_pos = (g_console_device->buffer_pos + g_console_device->buffer_size) % INPUT_BUFFER_SIZE;
                    g_console_device->input_buffer[write_pos] = ascii;
                    g_console_device->buffer_size++;
                    // 显示输入的字符
                    Console::putchar(ascii);
                }
            }
        }
    });
}

ConsoleDevice::~ConsoleDevice()
{
    if (g_console_device == this) {
        g_console_device = nullptr;
    }
}

ssize_t ConsoleDevice::read(void* buffer, size_t size)
{
    if (buffer_size == 0) {
        return 0;  // 没有可读取的数据
    }

    size_t bytes_to_read = size;
    if (bytes_to_read > buffer_size) {
        bytes_to_read = buffer_size;
    }

    char* char_buffer = static_cast<char*>(buffer);
    for (size_t i = 0; i < bytes_to_read; i++) {
        char_buffer[i] = input_buffer[(buffer_pos + i) % INPUT_BUFFER_SIZE];
    }

    buffer_pos = (buffer_pos + bytes_to_read) % INPUT_BUFFER_SIZE;
    buffer_size -= bytes_to_read;

    return bytes_to_read;
}

ssize_t ConsoleDevice::write(const void* buffer, size_t size)
{
    if (!buffer || size == 0) {
        return 0;
    }

    const char* chars = static_cast<const char*>(buffer);
    for (size_t i = 0; i < size; i++) {
        Console::putchar(chars[i]);
    }

    return size;
}

int ConsoleDevice::seek(size_t offset)
{
    return -1;  // 控制台设备不支持seek操作
}

int ConsoleDevice::close()
{
    return 0;  // 标准输入输出流不应该被关闭
}

} // namespace kernel