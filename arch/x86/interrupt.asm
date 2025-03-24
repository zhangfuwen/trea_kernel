; 中断处理程序入口点
[GLOBAL init_idts]
[GLOBAL enable_interrupts]
[GLOBAL disable_interrupts]
[GLOBAL remap_pic]
[EXTERN handleInterrupt]
[EXTERN handleSyscall]
[EXTERN save_context_wrapper]
[EXTERN restore_context_wrapper]

[section .data]
syscall_number: dd 0
arg1: dd 0
arg2: dd 0
arg3: dd 0

[section .text]

%macro SAVE_REGS 0
    push gs
    push fs
    push es
    push ds
    pushad
    push esp
    call save_context_wrapper
    add esp, 4
%endmacro

%macro RESTORE_REGS 0
    push esp
    call restore_context_wrapper
    add esp, 4
    popad
    pop ds
    pop es
    pop fs
    pop gs
%endmacro

%macro idtentry 3  ; 参数: vector, asm_code, c_handler
[global %2]
%2:
    cli
    SAVE_REGS

    push 0
    push %1
    call %3         ; 调用C函数
    add esp, 8      ; 清理错误码和中断号

    RESTORE_REGS
    sti
    iretd            ; 返回
%endmacro

; 定义具体中断
idtentry 0x20, timer_interrupt, handleInterrupt


[global syscall_interrupt]
syscall_interrupt:
    cli
    mov [syscall_number], eax
    mov [arg1], ebx
    mov [arg2], ecx
    mov [arg3], edx
    SAVE_REGS

    mov edx, [arg3]
    push edx ; arg3
    mov ecx, [arg2]
    push ecx ; arg2
    mov ebx, [arg1]
    push ebx ; arg1
    mov eax, [syscall_number]
    push eax ; syscall number
    call handleSyscall         ; 调用C函数
    add esp, 16

    RESTORE_REGS
    sti
    iretd            ; 返回

remap_pic:
    ; 初始化主PIC
    mov al, 0x11    ; 初始化命令
    out 0x20, al
    mov al, 0x20    ; 起始中断向量号
    out 0x21, al
    mov al, 0x04    ; 告诉主PIC从PIC连接在IRQ2
    out 0x21, al
    mov al, 0x01    ; 8086模式
    out 0x21, al

    ; 初始化从PIC
    mov al, 0x11    ; 初始化命令
    out 0xA0, al
    mov al, 0x28    ; 起始中断向量号
    out 0xA1, al
    mov al, 0x02    ; 告诉从PIC连接到主PIC的IRQ2
    out 0xA1, al
    mov al, 0x01    ; 8086模式
    out 0xA1, al

    ; 清除PIC掩码
    mov al, 0x0
    out 0x21, al
    out 0xA1, al
    ret

enable_interrupts:
    sti
    ret

disable_interrupts:
    cli
    ret
