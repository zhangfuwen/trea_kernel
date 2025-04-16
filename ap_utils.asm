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

hex_digits:
    db '0123456789ABCDEF'
