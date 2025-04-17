ENTRY(_start)

SECTIONS {
    . = 1M;

    .multiboot BLOCK(4K) : ALIGN(4K) {
        *(.multiboot)
        *(.text1)
    }

    . = 3M;
    .text BLOCK(4K) : ALIGN(4K) {
        *(.text)
    }
    .text.ap_boot BLOCK(4K) : ALIGN(4K) {
        *(.ap_boot16)
    }

    .rodata BLOCK(4K) : ALIGN(4K) {
        *(.rodata)
    }

    .data BLOCK(4K) : ALIGN(4K) {
        *(.data)
    }
    .percpu : ALIGN(4096) {
        __percpu_start = .;
        *(.percpu)
        *(.percpu.extended)
        __percpu_end = .;
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