#include "kernel/console_device.h"
#include "kernel/vfs.h"
#include "lib/console.h"
#include <arch/x86/interrupt.h>
#include <drivers/keyboard.h>
#include <lib/debug.h>

namespace kernel
{
ConsoleDevice* ConsoleFS::g_console_device = nullptr;

ConsoleDevice::ConsoleDevice() {}

ConsoleDevice::~ConsoleDevice() {}

int ConsoleDevice::iterate(void* buffer, size_t buffer_size, uint32_t* pos)
{
    return -1; // 字符设备不支持目录遍历
}

ssize_t ConsoleDevice::read(void* buffer, size_t size)
{
    // debug_debug("read: %x\n", size);
    auto scancode = keyboard_get_scancode();
    auto ascii = scancode_to_ascii(scancode);
    if(ascii == 0) {
        // debug_debug("keyboard_get_scancode: %x\n", scancode);
        return 0;
    }
    // debug_debug(
    //     "keyboard_get_scancode: %x, ascii:%c(0x%x), buffer:0x%x\n", scancode, ascii, ascii, buffer);

    ((uint8_t*)buffer)[0] = ascii;
    return 1;

    if(buffer_size == 0) {
        // debug_debug("no data to read\n");
        return 0; // 没有可读取的数据
    }

    size_t bytes_to_read = size;
    if(bytes_to_read > buffer_size) {
        bytes_to_read = buffer_size;
    }

    char* char_buffer = static_cast<char*>(buffer);
    for(size_t i = 0; i < bytes_to_read; i++) {
        char_buffer[i] = input_buffer[(read_pos + i) % INPUT_BUFFER_SIZE];
    }

    read_pos = (read_pos + bytes_to_read) % INPUT_BUFFER_SIZE;
    buffer_size -= bytes_to_read;
    hexdump(buffer, bytes_to_read, [](const char* str) { debug_err("%s", str); });
    debug_err("read %d bytes, %s\n", bytes_to_read, char_buffer);

    return bytes_to_read;
}

ssize_t ConsoleDevice::write(const void* buffer, size_t size)
{
    if(!buffer || size == 0) {
        return 0;
    }

    const char* chars = static_cast<const char*>(buffer);
    for(size_t i = 0; i < size; i++) {
        Console::putchar(chars[i]);
    }

    return size;
}

int ConsoleDevice::seek(size_t offset)
{
    return -1; // 控制台设备不支持seek操作
}

int ConsoleDevice::close()
{
    return 0; // 标准输入输出流不应该被关闭
}

} // namespace kernel