#include <stdint.h>

// 键盘端口
#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64

// 键盘状态标志
#define KEYBOARD_OUTPUT_FULL 0x01

// 从端口读取一个字节
static inline uint8_t inb(uint16_t port)
{
    uint8_t ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// 读取键盘状态
static uint8_t keyboard_read_status()
{
    return inb(KEYBOARD_STATUS_PORT);
}

// 读取键盘数据
static uint8_t keyboard_read_data()
{
    while(!(keyboard_read_status() & KEYBOARD_OUTPUT_FULL))
        ;
    return inb(KEYBOARD_DATA_PORT);
}

// 初始化键盘
void keyboard_init()
{
    // 清空键盘缓冲区
    while(keyboard_read_status() & KEYBOARD_OUTPUT_FULL) {
        keyboard_read_data();
    }
}

// 获取按键扫描码
uint8_t keyboard_get_scancode()
{
    return keyboard_read_data();
}