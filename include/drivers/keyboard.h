#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

// 按下
#define KEYBOARD_KEY_DOWN 0x80
#define KEYBOARD_KEY_UP 0x40
#define KEYBOARD_KEY_PRESSED 0x20
#define KEYBOARD_KEY_RELEASED 0x10

char scancode_to_ascii(unsigned char scancode);
void keyboard_init();
uint8_t keyboard_get_scancode();

#endif //KEYBOARD_H
