# x86架构相关的CMake配置

option(USE_APIC "Use APIC instead of PIC8259" ON)

# 创建x86架构库
add_library(arch_x86 STATIC
        apic.cpp
        cpu.cpp
        gdt.cpp
        gdt_load.asm
        idt.cpp
        interrupt.asm
        interrupt.cpp
        interrupt_names.c
        paging_asm.asm
        pic8259.cpp
        segment_fault.cpp
        smp.cpp
        spinlock.cpp

)

# 为interrupt_names.c设置C语言编译标准
set_source_files_properties(
        interrupt_names.c
        PROPERTIES
        LANGUAGE C
        COMPILE_FLAGS "-std=c99"
)

#if(USE_APIC)
    target_sources(arch_x86 PRIVATE apic.cpp pic8259.cpp)
#else()
#    target_sources(arch_x86 PRIVATE pic8259.cpp)
#endif()

# 设置汇编器
set_source_files_properties(
    gdt_load.asm
    paging_asm.asm
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
        ${OS_COMPILE_OPTIONS}
    -ffreestanding
    -O2
    -Wall
    -Wextra
    -m32
)