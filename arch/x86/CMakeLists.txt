# x86架构相关的CMake配置

# 创建x86架构库
add_library(arch_x86
    boot.asm
    gdt.cpp
    gdt_load.asm
    paging_asm.asm
    rootfs.asm
    idt.cpp
    interrupt.cpp
    paging.cpp
)

# 设置汇编器
set_source_files_properties(
    boot.asm
    gdt_load.asm
    paging_asm.asm
    rootfs.asm
    PROPERTIES
    LANGUAGE ASM_NASM
    COMPILE_FLAGS "-f elf32 -g -F dwarf"
)

# 添加包含目录
target_include_directories(arch_x86 PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# 编译选项
target_compile_options(arch_x86 PRIVATE
    -ffreestanding
    -O2
    -Wall
    -Wextra
    -m32
)