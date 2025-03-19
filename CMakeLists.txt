cmake_minimum_required(VERSION 3.10)
project(CustomKernel)

# 启用ASM_NASM语言支持
enable_language(ASM_NASM)

# 设置NASM编译器输出格式
set(CMAKE_ASM_NASM_OBJECT_FORMAT elf32)
set(CMAKE_ASM_NASM_COMPILE_OBJECT "<CMAKE_ASM_NASM_COMPILER> -f ${CMAKE_ASM_NASM_OBJECT_FORMAT} -o <OBJECT> <SOURCE>")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffreestanding -O2 -Wall -Wextra -m32 -I/usr/include/c++/13/i386-linux-gnu -I/usr/include/i386-linux-gnu")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -T ${CMAKE_SOURCE_DIR}/arch/x86/linker.ld -ffreestanding -O2 -nostdlib -m32 -no-pie")

# 添加子目录
add_subdirectory(arch)
add_subdirectory(kernel)
add_subdirectory(drivers)
add_subdirectory(lib)

# 主内核目标
add_executable(kernel.bin
    kernel/main.cpp
)

# 链接所有模块
target_link_libraries(kernel.bin
    arch_x86
    kernel_core
    drivers_core
    kernel_lib
)

# 自定义目标：创建ISO镜像
add_custom_target(kernel.iso
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/iso/boot/grub
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/kernel.bin ${CMAKE_BINARY_DIR}/iso/boot/
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/boot/grub.cfg ${CMAKE_BINARY_DIR}/iso/boot/grub/
    COMMAND grub-mkrescue -o ${CMAKE_BINARY_DIR}/kernel.iso ${CMAKE_BINARY_DIR}/iso
    DEPENDS kernel.bin
)