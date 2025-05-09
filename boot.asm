section .multiboot
    align 4
multiboot_header:
    dd 0x1BADB002                ; Multiboot magic number
    dd 0x00000003                ; Flags (align + memory info)
    dd -(0x1BADB002 + 0x00000003) ; Checksum

section .text

    global _start
    extern kernel_main
extern copy_ap_boot_to_8k
extern copy_ap_boot_to_0k
extern ap_entry
_start:
    cli                         ; 禁用中断
    mov esp, stack_top          ; 设置栈指针
    mov ebp, stack_top          ; 设置栈指针

    call copy_ap_boot_to_8k
;    call copy_ap_boot_to_0k

    ; 将 ap_entry 函数地址存储到预定义的内存位置
    mov dword [0x7E00], ap_entry

    ; 调用C++内核主函数
    call kernel_main

    ; 如果kernel_main返回，进入无限循环
.hang:
    hlt
    jmp .hang

extern ap_entry_asm
extern ap_entry_asm_end
copy_ap_boot_to_8k:
    mov esi, ap_entry_asm
    mov eax, ap_entry_asm_end
    sub eax, esi
    mov ecx, eax
    mov edi, 0x8000
rep movsb
ret

copy_ap_boot_to_0k:
    mov esi, ap_entry_asm
    mov eax, ap_entry_asm_end
    sub eax, esi
    mov ecx, eax
    mov edi, 0x0000
rep movsb
ret

section .bss
    align 16
    stack_bottom:
    resb 16384                  ; 16KB栈空间
    stack_top: