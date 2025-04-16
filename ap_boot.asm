BITS 16

global ap_entry_asm
global ap_entry_asm_end
section .text_16

ap_entry_asm:
    cli                     ; 禁用中断
    cld                     ; 清除方向标志
    mov ax, 0               ; 设置数据段
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00          ; 设置堆栈指针

    ; 调试输出 - 显示 AP 启动信息
    mov si, 0x8000 + 0x100  ; 假设消息数据在代码起始后的固定偏移位置
    call print_string

    ; 加载临时 GDT
    mov bx, 0x8000 + 0x140   ; 调整 GDT 描述符的偏移位置，确保与实际位置匹配
    lgdt [bx]

    ; 调试输出 - 显示 GDT 加载完成
    mov si, 0x8000 + 0x120  ; 假设第二个消息数据在代码起始后的固定偏移位置
    call print_string

    ; 启用保护模式
    mov eax, cr0
    or eax, 1               ; 设置 PE 位
    mov cr0, eax

    ; 远跳转到保护模式代码，使用硬编码地址而不是标签
    jmp 0x08:(0x8000 + 0x80)  ; 假设 protected_mode 在代码起始后的固定偏移位置

; 打印字符串函数 (实模式下)
print_string:
    mov ah, 0x0E            ; BIOS teletype 输出
.next_char:
    lodsb                   ; 加载字符
    cmp al, 0               ; 检查字符串结束
    je .done
    int 0x10                ; 调用 BIOS 中断
    jmp .next_char
.done:
    ret

BITS 32
; 确保这个位置与上面跳转地址的偏移量匹配
times 0x80 - ($ - ap_entry_asm) db 0
protected_mode:
    ; 设置数据段寄存器
    mov ax, 0x10            ; 数据段选择子
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; 调试信息 - 我们已进入保护模式
    ; 注意: 这里不能再使用 BIOS 中断，需要更复杂的显示方式
    ; 暂时使用简单的死循环作为标记
    mov ebx, 0xB8000       ; 视频内存地址
    mov byte [ebx], 'P'     ; 显示 'P' 表示保护模式
    mov byte [ebx+1], 0x0F  ; 白色文字

    ; TODO: 跳转到 AP 初始化代码
    jmp $

; 填充数据，确保在固定偏移位置
times 0x100 - ($ - ap_entry_asm) db 0
; 调试消息
db 'AP Starting...', 0
times 0x120 - ($ - ap_entry_asm) db 0
db 'GDT Loaded.', 0
times 0x140 - ($ - ap_entry_asm) db 0
; GDT 描述符
dw 0x0018       ; GDT 大小 (3 个描述符 * 8 字节 = 24 字节)
dd 0x8000 + 0x148  ; GDT 起始地址，紧跟在描述符之后
; 临时 GDT
times 0x148 - ($ - ap_entry_asm) db 0
dd 0x00000000
dd 0x00000000
dw 0xFFFF       ; limit 0-15
dw 0x0000       ; base 0-15
db 0x00         ; base 16-23
db 0x9A         ; access byte (present, ring 0, code segment, executable, readable)
db 0xCF         ; flags (4KB granularity, 32-bit mode) + limit 16-19
db 0x00         ; base 24-31
dw 0xFFFF       ; limit 0-15
dw 0x0000       ; base 0-15
db 0x00         ; base 16-23
db 0x92         ; access byte (present, ring 0, data segment, writable)
db 0xCF         ; flags (4KB granularity, 32-bit mode) + limit 16-19
db 0x00         ; base 24-31

ap_entry_asm_end:
    jmp $