; 中断处理程序入口点
[GLOBAL init_idts]
[GLOBAL enable_interrupts]
[GLOBAL disable_interrupts]
[GLOBAL remap_pic]
[EXTERN handleInterrupt]

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


%macro idtentry 3  ; 参数: vector, asm_code, c_handler
[global %2]
%2:
    cli

    push gs
    push fs
    push es
    push ds
    pushad

    push 0
    push %1

;    mov eax, 0x20
;    mov [eax], esp  ; 保存当前栈指针
    call %3         ; 调用C函数

    add esp, 8      ; 清理错误码和中断号

    popad
    pop ds
    pop es
    pop fs
    pop gs

    sti
    iretd            ; 返回
%endmacro


; 定义具体中断
idtentry 0x20, timer_interrupt, handleInterrupt
idtentry 0x80, syscall_interrupt, handleInterrupt

