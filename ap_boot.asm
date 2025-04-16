extern gdt_entries

global ap_entry_asm
global ap_entry_asm_end
;extern print_register_hex

section .text_16
;bits 16
align 4096

ap_entry_asm:
    ; 禁用中断
    cli
    mov esp, stack_top          ; 设置栈指针

;    call init_serial

    ;;;;;;;;;;;;;;;;;; 切换到保护模式 ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ; 动态计算 new_gdt_descriptor 的绝对地址
    mov eax, my_gdt_descriptor - ap_entry_asm ; 使用相对寻址获取 new_gdt_descriptor 相对于当前 EIP 的偏移
    ; 加载 GDT
    lgdt [eax]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    ; 切换到保护模式完成

    ;;;;;;;;;;;;;;;;; 打印字符 ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    mov dx, 0x3F8      ; Transmitter holding register
    mov al, 'a'        ; The character to print
    out dx, al         ; Output the character

    ;;;;;;;;;;;;;;;;;; 打印寄存器值 ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;    call print_eip

;print_eip:
;    pop eax
;    call print_register_hex

;loop:
    ;mov al, '0'        ; The character to print
    ;call print_char
    ;mov dx, 0x3F8        ; 发送保持寄存器
    ;out dx, al           ; 输出字符
;    jmp loop

    ;;;;;;;;;;;;;;;;;;;;;;;; 跳转到新的代码段 ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    extern ap_entry
    jmp 0x08:ap_entry

; GDT表
align 8
gdt_descriptor:
    dw (7 * 8) - 1                    ; GDT大小 (7个表项 * 8字节 - 1)
    dd gdt_entries                    ; GDT地址

.done_16:
    pop dx
    pop ax

; 新的 GDT 表项结构定义
; 段描述符结构（8 字节）
; 定义 GDT 表项的宏，按照 Linux 风格
%macro GDT_ENTRY 6
    dw (%2 & 0xFFFF)          ; 段界限 0 - 15 位
    dw (%1 & 0xFFFF)          ; 基地址 0 - 15 位
    db ((%1 >> 16) & 0xFF)    ; 基地址 16 - 23 位
    db (%4 << 4) | (%3 & 0x0F); 类型和特权级等访问权限
    db ((%2 >> 16) & 0x0F) | (%5 << 4); 段界限 16 - 19 位和标志位
    db ((%1 >> 24) & 0xFF)    ; 基地址 24 - 31 位
%endmacro

; 新的 GDT 表，包含 7 个描述符
align 8
new_gdt:
    ; 0 号描述符：空描述符，必须全 0
    dd 0
    dd 0
    ; 1 号描述符：内核代码段描述符
    ; 基地址: 0x00000000，段界限: 0xFFFFF，粒度位设置为 1（4KB 粒度），所以实际段界限是 0xFFFFF * 4KB = 4GB - 1
    ; 覆盖 4G 地址空间，段选择子值为 0x08（索引 1，特权级 0，表指示位 0）
    GDT_ENTRY 0, 0xFFFFF, 0xA, 0x9, 0xC, 0
    ; 2 号描述符：内核数据段描述符
    ; 基地址: 0x00000000，段界限: 0xFFFFF，粒度位设置为 1（4KB 粒度），所以实际段界限是 0xFFFFF * 4KB = 4GB - 1
    ; 覆盖 4G 地址空间，段选择子值为 0x10（索引 2，特权级 0，表指示位 0）
    GDT_ENTRY 0, 0xFFFFF, 0x2, 0x9, 0xC, 0
    ; 3 号描述符：用户代码段描述符
    ; 基地址: 0x00000000，段界限: 0xFFFFF，粒度位设置为 1（4KB 粒度），所以实际段界限是 0xFFFFF * 4KB = 4GB - 1
    ; 覆盖 4G 地址空间，段选择子值为 0x1B（索引 3，特权级 3，表指示位 0）
    GDT_ENTRY 0, 0xFFFFF, 0xA, 0xF, 0xC, 0
    ; 4 号描述符：用户数据段描述符
    ; 基地址: 0x00000000，段界限: 0xFFFFF，粒度位设置为 1（4KB 粒度），所以实际段界限是 0xFFFFF * 4KB = 4GB - 1
    ; 覆盖 4G 地址空间，段选择子值为 0x23（索引 4，特权级 3，表指示位 0）
    GDT_ENTRY 0, 0xFFFFF, 0x2, 0xF, 0xC, 0
    ; 5 号描述符：TSS 描述符（这里简单示例，实际使用需填充 TSS 信息）
    ; 此描述符不是覆盖 4G 空间的平坦模式描述符，用于任务状态段管理
    ; 段选择子值为 0x28（索引 5，特权级 0，表指示位 0）
    GDT_ENTRY 0, 0x67, 0x9, 0x89, 0x4, 0
    dd new_gdt            ; GDT 基地址
    dw ($ - new_gdt) - 1  ; GDT 大小，总字节数减 1
    dd new_gdt            ; GDT 基地址
    ; 此描述符不是覆盖 4G 空间的平坦模式描述符
    ; 段选择子值为 0x30（索引 6，特权级 0，表指示位 0）
    ; 6 号描述符：可按需添加其他描述符，这里先填充和 5 号类似的内容
    GDT_ENTRY 0, 0x67, 0x9, 0x89, 0x4, 0

; 新的 GDT 描述符
align 8
new_gdt_descriptor:
    dw $ - new_gdt - 1  ; GDT 大小
    dd new_gdt          ; GDT 基地址

align 8
stack_bottom:
    resb 100                  ; 16KB栈空间
stack_top:

; 定义 GDT 表
gdt_start:
    ; 空描述符，必须存在
    dd 0x0
    dd 0x0

; 代码段描述符
gdt_code:
    dw 0xFFFF       ; 段界限低 16 位
    dw 0x0          ; 段基地址低 16 位
    db 0x0          ; 段基地址中 8 位
    db 10011010b    ; 访问权限字节
    db 11001111b    ; 粒度和段界限高 4 位
    db 0x0          ; 段基地址高 8 位

; 数据段描述符
gdt_data:
    dw 0xFFFF       ; 段界限低 16 位
    dw 0x0          ; 段基地址低 16 位
    db 0x0          ; 段基地址中 8 位
    db 10010010b    ; 访问权限字节
    db 11001111b    ; 粒度和段界限高 4 位
    db 0x0          ; 段基地址高 8 位

gdt_end:

; GDT 描述符（用于 lgdt 指令）
my_gdt_descriptor:
    dw gdt_end - gdt_start - 1 ; GDT 界限
    dd gdt_start               ; GDT 基地址


;;;;;;;;;;;;;;;;;;;;; include "ap_utils.asm"
global print_register_hex
; 串口初始化函数
; 初始化 COM1 串口，波特率 9600，8 位数据位，1 位停止位，无校验位
init_serial:
    mov dx, 0x3F8 + 3  ; 线路控制寄存器
    mov al, 0x80       ; 允许访问除数锁存寄存器
    out dx, al

    mov dx, 0x3F8      ; 除数锁存寄存器低字节
    mov al, 0x0C       ; 波特率 9600 的低字节
    out dx, al

    mov dx, 0x3F8 + 1  ; 除数锁存寄存器高字节
    mov al, 0x00       ; 波特率 9600 的高字节
    out dx, al

    mov dx, 0x3F8 + 3  ; 线路控制寄存器
    mov al, 0x03       ; 8 位数据位，1 位停止位，无校验位
    out dx, al

    mov dx, 0x3F8 + 2  ; FIFO 控制寄存器
    mov al, 0x07       ; 启用 FIFO，清除接收和发送 FIFO，设置中断触发级别为 14 字节
    out dx, al

    mov dx, 0x3F8 + 4  ; 调制解调器控制寄存器
    mov al, 0x03       ; 启用 DTR 和 RTS
    out dx, al

    ret

; 函数：将寄存器的值以十六进制形式通过串口打印
; 输入：eax 寄存器存储要打印的值
print_register_hex:
    push ebx
    push ecx
    push edx
    push esi

    mov esi, hex_digits  ; 指向十六进制字符表
    mov ecx, 8           ; 8 个十六进制字符（32 位寄存器）

print_loop:
    rol eax, 4           ; 循环左移 4 位，获取下一个 4 位值
    mov bl, al           ; 保存低 8 位
    and bl, 0x0F         ; 提取低 4 位
    mov bl, [esi + ebx]  ; 获取对应的十六进制字符

    mov dx, 0x3F8 + 5    ; 行状态寄存器
check_status_print:
    in al, dx            ; 读取行状态
    test al, 0x20        ; 检查发送保持寄存器是否为空
    jz check_status_print; 如果不为空，继续检查

    mov dx, 0x3F8        ; 发送保持寄存器
    mov al, bl           ; 要打印的字符
    out dx, al           ; 输出字符

    loop print_loop

    pop esi
    pop edx
    pop ecx
    pop ebx
    ret

print_char:
    mov dx, 0x3F8        ; 发送保持寄存器
    out dx, al           ; 输出字符
    ret

hex_digits:
    db '0123456789ABCDEF'

ap_entry_asm_end:
