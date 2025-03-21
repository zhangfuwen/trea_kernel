[BITS 32]

section .text1
jmp 0x300000

section .text
global loadGDT_ASM

loadGDT_ASM:
    ; 从栈中获取GDTPointer的地址
    mov eax, [esp + 4]
    lgdt [eax]

    ; 重新加载段寄存器
    jmp 0x08:.reload_segments

.reload_segments:
    mov eax, 0x10
    mov ds, eax
    mov es, eax
    mov fs, eax
    mov gs, eax
    mov ss, eax
    ret