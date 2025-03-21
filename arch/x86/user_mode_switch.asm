; 用户态切换的汇编入口点
[GLOBAL user_mode_entry]

; 声明外部C++函数
[EXTERN user_mode_switch_handler]

; 调用门入口点
user_mode_entry:
    ; 保存寄存器状态
    push ebp
    mov ebp, esp
    pushad
    
    ; 调用C++处理函数
    call user_mode_switch_handler
    
    ; 恢复寄存器状态（实际上不会执行到这里，因为handler会通过iret跳转到用户态）
    popad
    pop ebp
    ret

; 添加.note.GNU-stack节，表示不需要可执行栈
section .note.GNU-stack noalloc noexec nowrite progbits