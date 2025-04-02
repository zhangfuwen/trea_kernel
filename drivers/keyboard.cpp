#include <lib/debug.h>
#include <lib/ioport.h>
#include <stdint.h>

// 键盘端口
#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64



// 键盘状态标志
#define KEYBOARD_OUTPUT_FULL 0x01

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

// 记录 Shift 键状态
volatile int shift_pressed = 0;

// 普通字符映射表（无 Shift 按下）
char normal_scancode_map[256];

// 在代码中手动设置元素的值
void init_normal_scancode_map()
{
    normal_scancode_map[0x1E] = 'a';
    normal_scancode_map[0x30] = 'b';
    normal_scancode_map[0x2E] = 'c';
    normal_scancode_map[0x20] = 'd';
    normal_scancode_map[0x12] = 'e';
    normal_scancode_map[0x21] = 'f';
    normal_scancode_map[0x22] = 'g';
    normal_scancode_map[0x23] = 'h';
    normal_scancode_map[0x17] = 'i';
    normal_scancode_map[0x24] = 'j';
    normal_scancode_map[0x25] = 'k';
    normal_scancode_map[0x26] = 'l';
    normal_scancode_map[0x32] = 'm';
    normal_scancode_map[0x2D] = 'n';
    normal_scancode_map[0x31] = 'n';
    normal_scancode_map[0x18] = 'o';
    normal_scancode_map[0x19] = 'p';
    normal_scancode_map[0x10] = 'q';
    normal_scancode_map[0x13] = 'r';
    normal_scancode_map[0x1F] = 's';
    normal_scancode_map[0x14] = 't';
    normal_scancode_map[0x16] = 'u';
    normal_scancode_map[0x2F] = 'v';
    normal_scancode_map[0x2C] = 'w';
    normal_scancode_map[0x2B] = 'x';
    normal_scancode_map[0x15] = 'y';
    normal_scancode_map[0x2A] = 'z';
    normal_scancode_map[0x02] = '1';
    normal_scancode_map[0x03] = '2';
    normal_scancode_map[0x04] = '3';
    normal_scancode_map[0x05] = '4';
    normal_scancode_map[0x06] = '5';
    normal_scancode_map[0x07] = '6';
    normal_scancode_map[0x08] = '7';
    normal_scancode_map[0x09] = '8';
    normal_scancode_map[0x0A] = '9';
    normal_scancode_map[0x0B] = '0';
    normal_scancode_map[0x0C] = '-';
    normal_scancode_map[0x0D] = '=';
    normal_scancode_map[0x1A] = '[';
    normal_scancode_map[0x1B] = ']';
    normal_scancode_map[0x29] = ';';
    normal_scancode_map[0x28] = '\'';
    normal_scancode_map[0x33] = ',';
    normal_scancode_map[0x34] = '.';
    normal_scancode_map[0x35] = '/';
    normal_scancode_map[0x0E] = '\b';
    normal_scancode_map[0x11] = '\t';
    normal_scancode_map[0x1C] = '\n';
    normal_scancode_map[0x39] = ' ';
}

char shift_scancode_map[256];

// 在代码中手动设置元素的值
void init_shift_scancode_map() {
    shift_scancode_map[0x1E] = 'A';
    shift_scancode_map[0x30] = 'B';
    shift_scancode_map[0x2E] = 'C';
    shift_scancode_map[0x20] = 'D';
    shift_scancode_map[0x12] = 'E';
    shift_scancode_map[0x21] = 'F';
    shift_scancode_map[0x22] = 'G';
    shift_scancode_map[0x23] = 'H';
    shift_scancode_map[0x17] = 'I';
    shift_scancode_map[0x24] = 'J';
    shift_scancode_map[0x25] = 'K';
    shift_scancode_map[0x26] = 'L';
    shift_scancode_map[0x32] = 'M';
    shift_scancode_map[0x2D] = 'N';
    shift_scancode_map[0x31] = 'N';
    shift_scancode_map[0x18] = 'O';
    shift_scancode_map[0x19] = 'P';
    shift_scancode_map[0x10] = 'Q';
    shift_scancode_map[0x13] = 'R';
    shift_scancode_map[0x1F] = 'S';
    shift_scancode_map[0x14] = 'T';
    shift_scancode_map[0x16] = 'U';
    shift_scancode_map[0x2F] = 'V';
    shift_scancode_map[0x2C] = 'W';
    shift_scancode_map[0x2B] = 'X';
    shift_scancode_map[0x15] = 'Y';
    shift_scancode_map[0x2A] = 'Z';
    shift_scancode_map[0x02] = '!';
    shift_scancode_map[0x03] = '@';
    shift_scancode_map[0x04] = '#';
    shift_scancode_map[0x05] = '$';
    shift_scancode_map[0x06] = '%';
    shift_scancode_map[0x07] = '^';
    shift_scancode_map[0x08] = '&';
    shift_scancode_map[0x09] = '*';
    shift_scancode_map[0x0A] = '(';
    shift_scancode_map[0x0B] = ')';
    shift_scancode_map[0x0C] = '_';
    shift_scancode_map[0x0D] = '+';
    shift_scancode_map[0x1A] = '{';
    shift_scancode_map[0x1B] = '}';
    shift_scancode_map[0x29] = ':';
    shift_scancode_map[0x28] = '"';
    shift_scancode_map[0x33] = '<';
    shift_scancode_map[0x34] = '>';
    shift_scancode_map[0x35] = '?';
    shift_scancode_map[0x0E] = '\b';
    shift_scancode_map[0x11] = '\t';
    shift_scancode_map[0x1C] = '\n';
    normal_scancode_map[0x39] = ' ';
}


// 在合适的地方调用初始化函数
// 例如在 keyboard_init 函数中
void keyboard_init()
{
    // 清空键盘缓冲区
    while(keyboard_read_status() & KEYBOARD_OUTPUT_FULL) {
        keyboard_read_data();
    }
    init_normal_scancode_map();
    init_shift_scancode_map();
}


// 根据扫描码和 Shift 状态获取 ASCII 码
char scancode_to_ascii(unsigned char scancode) {
    if (scancode == 0x2A || scancode == 0x36) {
        // 左 Shift 或右 Shift 按下
        shift_pressed = 1;
        return 0;
    } else if (scancode == 0xAA || scancode == 0xB6) {
        // 左 Shift 或右 Shift 释放
        shift_pressed = 0;
        return 0;
    }

    if (shift_pressed) {
        return shift_scancode_map[scancode];
    } else {
        return normal_scancode_map[scancode];
    }
}

// 获取按键扫描码
uint8_t keyboard_get_scancode()
{
    return keyboard_read_data();
}