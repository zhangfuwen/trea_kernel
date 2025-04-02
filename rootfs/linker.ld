ENTRY(start)

SECTIONS {
    . = 1M;

    .text BLOCK(4K) : ALIGN(4K) {
        *(.text)
    }

    .rodata BLOCK(4K) : ALIGN(4K) {
        *(.rodata)
    }

    .data BLOCK(4K) : ALIGN(4K) {
        *(.data)
    }

    .bss BLOCK(4K) : ALIGN(4K) {
        *(COMMON)
        *(.bss)
    }

    .cmd_table BLOCK(4K) : ALIGN(4K) {
        __cmd_table_start = .;
        *(.cmd_table)
        __cmd_table_end = .;
    }

    .initramfs BLOCK(4K) : ALIGN(4K) {
        *(.initramfs)
    }

    /DISCARD/ : {
        *(.comment)
        *(.eh_frame)
        *(.note.gnu.build-id)
    }
}