; 中断处理程序入口点
[GLOBAL init_idts]
[GLOBAL enable_interrupts]
[GLOBAL disable_interrupts]
; 注意：remap_pic函数已弃用，系统现在使用APIC而不是PIC
; 保留此导出仅用于兼容性，不应在新代码中调用
[GLOBAL remap_pic]
[EXTERN handleInterrupt]
[EXTERN handleSyscall]
[EXTERN save_context_wrapper]
[EXTERN restore_context_wrapper]
[EXTERN page_fault_handler]
[EXTERN segmentation_fault_handler]
[EXTERN apic_send_eoi]

[section .data]
syscall_number: dd 0
arg1: dd 0
arg2: dd 0
arg3: dd 0
arg4: dd 0
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

    ; 对于APIC中断，C函数中已经调用了apic_send_eoi
    ; 所以这里不需要额外的EOI调用

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
idtentry 0x21, keyboard_interrupt, handleInterrupt
idtentry 0x2E, ide1_interrupt, handleInterrupt
idtentry 0x2F, ide2_interrupt, handleInterrupt

; APIC IPI中断处理
idtentry 0x40, ipi_interrupt, handleInterrupt  ; 处理器间中断
idtentry 0x41, ipi_reschedule, handleInterrupt ; 重新调度IPI

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
    mov [arg4], esi
    SAVE_REGS_FOR_CONTEXT_SWITCH

    mov esi, [arg4]
    push esi ; arg4
    mov edx, [arg3]
    push edx ; arg3
    mov ecx, [arg2]
    push ecx ; arg2
    mov ebx, [arg1]
    push ebx ; arg1
    mov eax, [syscall_number]
    push eax ; syscall number
    call handleSyscall         ; 调用C函数
    mov [arg1], eax
    add esp, 20

    RESTORE_REGS_FOR_CONTEXT_SWITCH
    mov eax, [arg1]
    sti
    iretd            ; 返回

remap_pic:
    ; 警告：此函数已弃用！系统现在使用APIC而不是PIC
    ; 此函数不应被调用，保留仅用于兼容性目的
    ; 如果需要初始化中断控制器，请使用arch::apic_init()和arch::ioapic_init()
    
    ; 为避免意外影响系统，此函数现在是空操作
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

