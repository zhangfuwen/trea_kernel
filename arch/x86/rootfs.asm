global __initramfs_start
global __initramfs_end

section .initramfs
__initramfs_start:
    incbin "/mnt/c/Users/刘涛/Documents/Code/custom_kernel/data/rootfs.cpio.newc"
    ; 定义一个字节数据
;    byte_data db 0x10  
    ; 定义一个字（2 字节）数据
;    word_data dw 0x1234  
    ; 定义一个双字（4 字节）数据
;    dword_data dd 0x12345678  
    ; 定义一个字符串
;    string_data db 'Hello, World!', 0  ; 0 表示字符串结束符
__initramfs_end: