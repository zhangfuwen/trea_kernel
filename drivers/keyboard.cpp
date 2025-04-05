#include <lib/debug.h>
#include <lib/ioport.h>
#include <cstdint>

// Linux input event structure
struct input_event {
    uint32_t time_sec;    // seconds
    uint32_t time_usec;   // microseconds
    uint16_t type;        // event type
    uint16_t code;        // event code
    int32_t value;        // event value
};

// Event types
#define EV_KEY          0x01
#define EV_REP          0x14

// Buffer size for keyboard events
#define EVENT_BUFFER_SIZE 32

// Circular buffer for keyboard events
static input_event event_buffer[EVENT_BUFFER_SIZE];
static int buffer_head = 0;
static int buffer_tail = 0;

// Add event to buffer
static void push_event(uint16_t type, uint16_t code, int32_t value) {
    input_event event;
    event.time_sec = 0;  // TODO: Implement real time
    event.time_usec = 0;
    event.type = type;
    event.code = code;
    event.value = value;

    event_buffer[buffer_tail] = event;
    buffer_tail = (buffer_tail + 1) % EVENT_BUFFER_SIZE;
}

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
volatile uint8_t precode = 0;

// 普通字符映射表（无 Shift 按下）
char normal_scancode_map[256];
char extended_scancode_map[256];

// 在代码中手动设置元素的值
void init_normal_scancode_map()
{
    extended_scancode_map[0x48] = 24 ; // 0xE0 0x48是向上箭头
    extended_scancode_map[0x50] = 25 ; // 0xE0 0x50是向下箭头
    extended_scancode_map[0x4D] = 26 ; // 0xE0 0x4D是向右箭头
    extended_scancode_map[0x4B] = 27 ; // 0xE0 0x4B是向左箭头

    // normal_scancode_map[0x52] = 0 ; // 0xE0 0x52是插入键
    extended_scancode_map[0x53] = 0 ; // 0xE0 0x53是删除键
    // normal_scancode_map[0x52] = 30 ; // 0xE0 0x52是 Home 键
    // normal_scancode_map[0x53] = 31 ; // 0xE0 0x53是 End 键
    // normal_scancode_map[0x54] = 32 ; // 0xE0 0x54是 Page Up 键
    // normal_scancode_map[0x55] = 33 ; // 0xE0 0x55是 Page Down 键

    normal_scancode_map[0x3b] = 0 ; //  0x3b是 F1 键
    normal_scancode_map[0x3c] = 0 ; //  0x3c是 F2 键
    normal_scancode_map[0x3d] = 0 ; //  0x3d是 F3 键
    normal_scancode_map[0x3e] = 0 ; //  0x3e是 F4 键
    normal_scancode_map[0x3f] = 0 ; //  0x3f是 F5 键
    normal_scancode_map[0x40] = 0 ; //  0x40是 F6 键
    normal_scancode_map[0x41] = 0 ; //  0x41是 F7 键
    normal_scancode_map[0x42] = 0 ; //  0x42是 F8 键
    normal_scancode_map[0x43] = 0 ; //  0x43是 F9 键
    normal_scancode_map[0x44] = 0 ; //  0x44是 F10 键
    normal_scancode_map[0x57] = 0 ; //  0x57是 F11 键
    normal_scancode_map[0x58] = 0 ; //  0x58是 F12 键

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
//    normal_scancode_map[0x2B] = 'x';
    normal_scancode_map[0x2D] = 'x';
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
    shift_scancode_map[0x39] = ' ';
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
    
    // 初始化事件缓冲区
    memset(event_buffer, 0, sizeof(event_buffer));
    buffer_head = buffer_tail = 0;

    // 启用键盘中断
    outb(0x21, inb(0x21) & ~0x02);
}


// 根据扫描码和 Shift 状态获取 ASCII 码
char scancode_to_ascii(unsigned char scancode) {
    debug_debug("scancode_to_ascii 0x%x\n", scancode);
    bool is_release = (scancode & 0x80) != 0;
    uint8_t ascii = 0;
    if (scancode == 0x2A || scancode == 0x36) {
        // 左 Shift 或右 Shift 按下
        shift_pressed = 1;
        goto out;
    } else if (scancode == 0xAA || scancode == 0xB6) {
        // 左 Shift 或右 Shift 释放
        shift_pressed = 0;
        goto out;
    }


    // // 生成input事件
    // push_event(EV_KEY, key_code, is_release ? 0 : 1);
    //
    if (!is_release) {
        if (shift_pressed) {
            ascii = shift_scancode_map[scancode];
        } else {
            ascii = normal_scancode_map[scancode];
        }
        if (precode == 0xE0) {
            ascii = extended_scancode_map[scancode];
        }
    }
out:
    precode = scancode;
    return ascii;
}

// 获取按键扫描码
uint8_t keyboard_get_scancode()
{
    return keyboard_read_data();
}