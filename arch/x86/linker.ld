ENTRY(_start)

SECTIONS {
    . = 1M;

    .text1 BLOCK(4K) : ALIGN(4K) {
        *(.multiboot)
        *(.text1)
    }

    . = 3M;
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

    .initramfs BLOCK(4K) : ALIGN(4K) {
        *(.initramfs)
    }

    /DISCARD/ : {
        *(.comment)
        *(.eh_frame)
        *(.note.gnu.build-id)
    }
}