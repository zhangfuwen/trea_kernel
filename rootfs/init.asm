global start
[extern main]
start:
    nop
    nop
    nop

    push eax
    pop ebx

;    mov esp, stack_top

    ; 跳转到main函数
    call main

    hlt

section .bss
[global stack_bottom]
[global stack_top]
stack_bottom:
    resb 4096  ; NASM使用resb代替.skip
stack_top: