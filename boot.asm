section .multiboot
    align 4
    dd 0x1BADB002                ; Multiboot magic number
    dd 0x00000003                ; Flags (align + memory info)
    dd -(0x1BADB002 + 0x00000003) ; Checksum

section .text

    global _start
    extern kernel_main

_start:
    cli                         ; 禁用中断
    mov esp, stack_top          ; 设置栈指针
    mov ebp, stack_top          ; 设置栈指针

    ; 调用C++内核主函数
    call kernel_main

    ; 如果kernel_main返回，进入无限循环
.hang:
    hlt
    jmp .hang

section .bss
    align 16
    stack_bottom:
    resb 16384                  ; 16KB栈空间
    stack_top: