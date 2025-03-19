#ifndef CONSOLE_H
#define CONSOLE_H

#include <cstdint>

void console_print(const char *msg);

class Console {
public:
    static void init();
    static void clear();
    static void putchar(char c);
    static void print(const char* str);
    static void setColor(uint8_t foreground, uint8_t background);

private:
    static const uint16_t VGA_WIDTH = 80;
    static const uint16_t VGA_HEIGHT = 25;
    static const uint32_t VGA_MEMORY = 0xB8000;

    static uint16_t* videoMemory;
    static uint16_t cursorX;
    static uint16_t cursorY;
    static uint8_t currentColor;

    static void updateCursor();
    static void scroll();
};

// VGA颜色定义
enum VGAColor {
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE = 1,
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_CYAN = 3,
    VGA_COLOR_RED = 4,
    VGA_COLOR_MAGENTA = 5,
    VGA_COLOR_BROWN = 6,
    VGA_COLOR_LIGHT_GREY = 7,
    VGA_COLOR_DARK_GREY = 8,
    VGA_COLOR_LIGHT_BLUE = 9,
    VGA_COLOR_LIGHT_GREEN = 10,
    VGA_COLOR_LIGHT_CYAN = 11,
    VGA_COLOR_LIGHT_RED = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_LIGHT_BROWN = 14,
    VGA_COLOR_WHITE = 15
};

#endif