cmake_minimum_required(VERSION 3.10)
project(CustomKernel)

# 启用ASM_NASM语言支持
enable_language(ASM_NASM)
set(CMAKE_VERBOSE_MAKEFILE ON)

# 设置NASM编译器输出格式
set(CMAKE_ASM_NASM_OBJECT_FORMAT elf32)
set(CMAKE_ASM_NASM_COMPILE_OBJECT "<CMAKE_ASM_NASM_COMPILER> -f ${CMAKE_ASM_NASM_OBJECT_FORMAT} -o <OBJECT> <SOURCE>")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 编译选项
# 移除原有的全局设置
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -Wall -Werror=return-type -Wextra -m32 -I/usr/include/c++/13/i386-linux-gnu -I/usr/include/i386-linux-gnu -Iinclude -nostartfiles -static-libstdc++ -static-libgcc -fno-exceptions -fno-rtti -fno-use-cxa-atexit")
# set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -O2 -m32 -no-pie -Wl,--build-id=none -static  -static-libstdc++ -static-libgcc")
# 为目标设置编译选项
set(OS_COMPILE_OPTIONS
        -ffreestanding
        -O2
        -Wall
        -Werror=return-type
        -Wextra
        -m32
        -I/usr/include/c++/13/i386-linux-gnu
        -I/usr/include/i386-linux-gnu
        -I${CMAKE_CURRENT_SOURCE_DIR}/include
        -nostartfiles
        -static-libstdc++
        -static-libgcc
        -fno-exceptions
        -fno-rtti
        -fno-use-cxa-atexit
        -fpermissive
)

# 为目标设置链接选项
set(OS_LINK_OPTIONS
        -ffreestanding
        -nostdlib
        -nodefaultlibs
        -O2
        -m32
        -no-pie
        -Wl,--build-id=none
        -static
        -static-libstdc++
        -static-libgcc
)



# 添加子目录
add_subdirectory(rootfs)
add_subdirectory(arch)
add_subdirectory(kernel)
add_subdirectory(drivers)
add_subdirectory(lib)
add_subdirectory(tests)

# 为其他目标（如果有）设置编译和链接选项
# 例如：
# target_compile_options(another_target PRIVATE ${TARGET_COMPILE_OPTIONS})
# target_link_options(another_target PRIVATE ${TARGET_LINK_OPTIONS})

# 创建一个自定义目标来执行脚本
add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/rootfs.cpio
        COMMAND ${CMAKE_COMMAND} -E echo "开始执行脚本...."
        COMMAND bash ${CMAKE_SOURCE_DIR}/tools/cpio_fs.sh ${CMAKE_BINARY_DIR}
        COMMAND ${CMAKE_COMMAND} -E echo "脚本执行完成。"
        COMMENT "每次构建前执行脚本"
        DEPENDS init
        VERBATIM
)
add_custom_target(run_cpio_fs_script
        DEPENDS ${CMAKE_BINARY_DIR}/rootfs.cpio
)

set(ROOTFS_CPIO_PATH "\"${CMAKE_BINARY_DIR}/\"")
#configure_file(
#        ${CMAKE_CURRENT_SOURCE_DIR}/rootfs.asm.in
#        ${CMAKE_CURRENT_BINARY_DIR}/rootfs.asm
#)
add_custom_command(
    OUTPUT ${CMAKE_SOURCE_DIR}/rootfs/binary/init
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/rootfs_bin/init ${CMAKE_SOURCE_DIR}/rootfs/binary/init
        DEPENDS init
        COMMENT "Copying init to rootfs/binary"
        VERBATIM
)
add_custom_target(copy_init_to_rootfs_disk
        DEPENDS ${CMAKE_SOURCE_DIR}/rootfs/binary/init
)

# 主内核目标
add_executable(kernel.bin
        rootfs.asm
        ap_boot.asm
        boot.asm
        linker.ld
        ${CMAKE_BINARY_DIR}/rootfs.cpio
)
# 为主内核目标设置编译和链接选项
target_compile_options(kernel.bin PRIVATE ${OS_COMPILE_OPTIONS})
target_link_options(kernel.bin PRIVATE ${OS_LINK_OPTIONS})
add_dependencies(kernel.bin run_cpio_fs_script)
#add_dependencies(kernel.bin ${CMAKE_BINARY_DIR}/rootfs.cpio)
target_link_options(kernel.bin PRIVATE "-T${CMAKE_CURRENT_SOURCE_DIR}/linker.ld")

# 设置汇编器
set_source_files_properties(
        rootfs.asm
        ap_boot.asm
        boot.asm
        PROPERTIES
        LANGUAGE ASM_NASM
        COMPILE_FLAGS "-f elf32 -g -F dwarf"
)
set_source_files_properties(
        ${CMAKE_BINARY_DIR}/rootfs.cpio
        PROPERTIES
        GENERATED TRUE
)

set(CMAKE_ASM_NASM_FLAGS "-I${ROOTFS_CPIO_PATH}")

add_dependencies(init kernel_core_main arch_x86 kernel_lib)
add_dependencies(kernel.bin init)

# 链接所有模块
# 使用 --whole-archive 选项链接 arch_x86 库
set_target_properties(kernel.bin PROPERTIES
    LINK_FLAGS "-Wl,--whole-archive arch/x86/libarch_x86.a -Wl,--no-whole-archive"
)

target_link_libraries(kernel.bin
        arch_x86
        kernel_core_main
        drivers_core
        kernel_lib
        kernel_smp
        kernel_fs
        kernel_process
        kernel_memory
)

# 自定义目标：创建ISO镜像
add_custom_target(kernel.iso
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/iso/boot/grub
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/kernel.bin ${CMAKE_BINARY_DIR}/iso/boot/
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/boot/grub.cfg ${CMAKE_BINARY_DIR}/iso/boot/grub/
        COMMAND grub-mkrescue -o ${CMAKE_BINARY_DIR}/kernel.iso ${CMAKE_BINARY_DIR}/iso
        DEPENDS kernel.bin
        DEPENDS copy_init_to_rootfs_disk
        DEPENDS run_cpio_fs_script
)