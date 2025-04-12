bits 16
section .ap_boot16
global ap_boot_entry
global ap_boot_entry_end
ap_boot_entry:
jmp ap_boot

; 定义字符串数据
msg_ap_start db "AP processor starting...", 0
msg_protected_mode db "Entering protected mode...", 0
msg_32bit_mode db "Now in 32-bit mode...", 0

; 串口相关常量
COM1_BASE equ 0x3F8

; 串口输出函数
serial_print:
    push ax
    push dx
.next_char:
    mov dx, COM1_BASE + 5   ; 线路状态寄存器
.wait_ready:
    in al, dx
    test al, 0x20          ; 测试发送保持寄存器是否为空
    jz .wait_ready         ; 如果不为空，继续等待
    mov dx, COM1_BASE      ; 数据寄存器
    mov al, [si]           ; 获取要打印的字符
    test al, al            ; 检查是否到字符串末尾
    jz .done
    out dx, al             ; 发送字符
    inc si                 ; 移动到下一个字符
    jmp .next_char
.done:
    pop dx
    pop ax
    ret

; GDT表
extern gdt_entries
gdt_descriptor:
    dw (7 * 8) - 1                    ; GDT大小 (7个表项 * 8字节 - 1)
    dd gdt_entries                    ; GDT地址

    bits 32

global ap_boot
ap_boot:
    ; 禁用中断
    cli
    ; 切换到保护模式
    lgdt [gdt_descriptor]
    mov eax, cr0
    or eax, 1
    mov cr0, eax

    ; 远跳转到32位代码
    jmp 0x08:ap_boot32
ap_boot_end:
ap_boot_entry_end:

section .text
ap_boot32:
    ; 打印启动消息
    lea si, [msg_ap_start]
    call serial_print

    ; 打印进入保护模式消息
    lea si, [msg_protected_mode]
    call serial_print

    ; 设置段寄存器
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; 设置栈
    mov esp, 0x90000

    ; 打印32位模式消息
    lea si, [msg_32bit_mode]
    call serial_print

    ; 跳转到C++的AP入口函数
    extern ap_entry
    jmp ap_entry

