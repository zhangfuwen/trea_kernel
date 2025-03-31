#ifndef SERIAL_H
#define SERIAL_H

void serial_init();
// 向串口发送一个字符
void serial_putc(char c);

// 向串口发送一个字符串
void serial_puts(const char* str);

#endif