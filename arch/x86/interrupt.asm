; 中断处理程序入口点
[GLOBAL init_idts]
[GLOBAL enable_interrupts]
[GLOBAL disable_interrupts]
[GLOBAL remap_pic]
[EXTERN handleInterrupt]
[EXTERN handleSyscall]
[EXTERN save_context_wrapper]
[EXTERN restore_context_wrapper]
[EXTERN page_fault_handler]
[EXTERN segmentation_fault_handler]

[section .data]
syscall_number: dd 0
arg1: dd 0
arg2: dd 0
arg3: dd 0
page_fault_errno: dd 0
segmentation_fault_errno: dd 0
fault_errno: dd 0

[section .text]

%macro SAVE_REGS_FOR_CONTEXT_SWITCH 0
    push gs
    push fs
    push es
    push ds
    pushad
    push esp
    call save_context_wrapper
    add esp, 4
%endmacro

%macro RESTORE_REGS_FOR_CONTEXT_SWITCH 0
    push esp
    call restore_context_wrapper
    add esp, 4
    popad
    pop ds
    pop es
    pop fs
    pop gs
%endmacro

%macro SAVE_REGS 0
    push gs
    push fs
    push es
    push ds
    pushad
%endmacro

%macro RESTORE_REGS 0
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
    SAVE_REGS_FOR_CONTEXT_SWITCH

    push 0
    push %1
    call %3         ; 调用C函数
    add esp, 8      ; 清理错误码和中断号

    RESTORE_REGS_FOR_CONTEXT_SWITCH
    sti
    iretd            ; 返回
%endmacro

%macro fault_errno 3  ; 参数: vector, asm_code, c_handler
[global %2]
[extern %3]
%2:
    cli
    push eax
    mov eax, [esp+4]
    mov [fault_errno], eax
    pop eax
    SAVE_REGS

    push dword [fault_errno]  ; 将错误码作为第二个参数
    push $1 ; 中断号作为第一个参数
    call $3
    add esp, 8      ; 清理参数

    RESTORE_REGS
    sti
    iretd            ; 返回
%endmacro

;fault_errno 13, general_protection_interrupt, general_fault_errno_handler ; General Protection Fault

[global general_protection_interrupt]
[extern general_protection_fault_handler]
general_protection_interrupt:
    cli
    push eax
    mov eax, [esp+4]
    mov [fault_errno], eax
    pop eax
    SAVE_REGS

    push dword [fault_errno]  ; 将错误码作为第二个参数
    push 13 ; 中断号作为第一个参数
    call general_protection_fault_handler
    add esp, 8      ; 清理参数

    RESTORE_REGS
    sti
    iretd            ; 返回

; 定义具体中断
idtentry 0x20, timer_interrupt, handleInterrupt

; 页面错误中断处理
[global page_fault_interrupt]
page_fault_interrupt:
    cli
    push eax
    mov eax, [esp+4]
    mov [page_fault_errno], eax
    pop eax
    SAVE_REGS
    
    mov eax, cr2    ; 获取故障地址
    push eax        ; 将故障地址作为第二个参数
    push dword [page_fault_errno]  ; 将错误码作为第一个参数
    call page_fault_handler
    add esp, 8      ; 清理参数

    push eax
    ; 保存当前 CR3 的值到 eax 寄存器
    mov eax, cr3
    ; 将 eax 中的值重新写回到 CR3 寄存器，触发 TLB 刷新
    mov cr3, eax
    pop eax
    
    RESTORE_REGS
    add esp, 4
    sti
    iretd


[global syscall_interrupt]
syscall_interrupt:
    cli
    mov [syscall_number], eax
    mov [arg1], ebx
    mov [arg2], ecx
    mov [arg3], edx
    SAVE_REGS_FOR_CONTEXT_SWITCH

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

    RESTORE_REGS_FOR_CONTEXT_SWITCH
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



; 段错误中断处理
[global segmentation_fault_interrupt]
segmentation_fault_interrupt:
    cli
    push eax
    mov eax, [esp+4]
    mov [segmentation_fault_errno], eax
    pop eax
    SAVE_REGS
    
    push dword [segmentation_fault_errno]  ; 将错误码作为第一个参数
    call segmentation_fault_handler
    add esp, 8      ; 清理参数

    push eax
    ; 保存当前 CR3 的值到 eax 寄存器
    mov eax, cr3
    ; 将 eax 中的值重新写回到 CR3 寄存器，触发 TLB 刷新
    mov cr3, eax
    pop eax
    
    RESTORE_REGS
    add esp, 4
    sti
    iretd

