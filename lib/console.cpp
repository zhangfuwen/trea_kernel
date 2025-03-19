#include "lib/console.h"
#include "lib/ioport.h"
#include "lib/serial.h"

void console_print(const char *msg) {
    Console::print(msg);
}

// 静态成员变量初始化
uint16_t* Console::videoMemory = nullptr;
uint16_t Console::cursorX = 0;
uint16_t Console::cursorY = 0;
uint8_t Console::currentColor = VGA_COLOR_LIGHT_GREY | (VGA_COLOR_BLACK << 4);

void Console::init() {
    // 确保在保护模式下初始化
    videoMemory = reinterpret_cast<uint16_t*>(VGA_MEMORY);
    clear();
    updateCursor();
}

void Console::clear() {
    uint16_t blank = ' ' | (currentColor << 8);
    for (uint16_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        videoMemory[i] = blank;
    }
    cursorX = 0;
    cursorY = 0;
    updateCursor();
}

void Console::putchar(char c) {
    if (!videoMemory) return;

    if (c == '\n') {
        cursorY++;
        cursorX = 0;
    } else if (c == '\r') {
        cursorX = 0;
    } else if (c == '\t') {
        cursorX = (cursorX + 8) & ~(8 - 1);
    } else {
        uint16_t position = cursorY * VGA_WIDTH + cursorX;
        if (position < VGA_WIDTH * VGA_HEIGHT) {
            videoMemory[position] = c | (currentColor << 8);
            cursorX++;
        }
    }

    if (cursorX >= VGA_WIDTH) {
        cursorX = 0;
        cursorY++;
    }

    if (cursorY >= VGA_HEIGHT) {
        scroll();
    }

    updateCursor();
}

void Console::print(const char* str) {
    serial_puts(str);
    while (*str) {
        putchar(*str++);
    }
}

void Console::setColor(uint8_t foreground, uint8_t background) {
    currentColor = foreground | (background << 4);
}

void Console::updateCursor() {
    if (!videoMemory) return;
    uint16_t position = cursorY * VGA_WIDTH + cursorX;

    // 使用封装的端口访问函数
    outb(0x3D4, 0x0F);
    outb(0x3D5, position & 0xFF);
    outb(0x3D4, 0x0E);
    outb(0x3D5, (position >> 8) & 0xFF);
}

void Console::scroll() {
    // 将所有行向上移动一行
    for (uint16_t i = 0; i < (VGA_HEIGHT - 1) * VGA_WIDTH; i++) {
        videoMemory[i] = videoMemory[i + VGA_WIDTH];
    }

    // 清空最后一行
    uint16_t blank = ' ' | (currentColor << 8);
    for (uint16_t i = (VGA_HEIGHT - 1) * VGA_WIDTH; i < VGA_HEIGHT * VGA_WIDTH; i++) {
        videoMemory[i] = blank;
    }

    cursorY = VGA_HEIGHT - 1;
}
