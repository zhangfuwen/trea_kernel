#include <stdint.h>

#include "lib/ioport.h"

// 定义串口 COM1 的端口地址
#define COM1_BASE 0x3F8

// 初始化串口
void serial_init()
{
    // 禁用中断
    outb(COM1_BASE + 1, 0x00);
    // 设置波特率为 38400
    outb(COM1_BASE + 3, 0x80);
    outb(COM1_BASE + 0, 0x03);
    outb(COM1_BASE + 1, 0x00);
    // 设置数据位为 8 位，无校验位，1 位停止位
    outb(COM1_BASE + 3, 0x03);
    // 启用 FIFO，设置 FIFO 触发级别为 14 字节
    outb(COM1_BASE + 2, 0xC7);
    // 启用 DTR、RTS
    outb(COM1_BASE + 4, 0x03);
}

// 向串口发送一个字符
void serial_putc(char c)
{
    // 等待发送缓冲区为空
    while((inb(COM1_BASE + 5) & 0x20) == 0)
        ;
    // 发送字符
    outb(COM1_BASE, c);
}

// 向串口发送一个字符串
void serial_puts(const char* str)
{
    while(*str) {
        serial_putc(*str++);
    }
}