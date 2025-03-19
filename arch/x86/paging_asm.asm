section .text
global cr0_asm
global cr3_asm

; 添加.note.GNU-stack节以避免可执行栈警告
section .note.GNU-stack noalloc noexec nowrite progbits

; 导出cr0和cr3符号
cr0_asm:
    mov eax, cr0
    ret

cr3_asm:
    mov eax, cr3
    ret